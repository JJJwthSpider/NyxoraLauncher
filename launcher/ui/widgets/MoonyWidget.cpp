// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "MoonyWidget.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <algorithm>
#include <cmath>

namespace {
enum Part { Head, Body, RightArm, LeftArm, Skirt, PartCount };
constexpr int s_namedParts = 4;
const char* const s_parts[s_namedParts] = { "head", "body", "rarm", "larm" };
constexpr float s_pi = 3.14159265f;

/// Arkan's model (resources/sculk_theme/arkan_model.json and arkan.png, made by android/art/arkan3d.py), loaded once
struct ArkanModel {
    QImage texture;  // ARGB32, not premultiplied: the pixels with alpha 254 glow
    float pivots[s_namedParts][3] = {};
    float hands[s_namedParts][3] = {};
    int magic = RightArm;  // the raised hand under the tesseract
    std::vector<std::array<float, 3>> eyes;
    std::vector<std::array<float, 5>> glows;  // part, x, y, z, radius: soft lights over the model (the ruby)
    std::vector<std::array<float, 10>> boxes;  // part, x, y, z, w, h, d, u, v, inflate
};

void readPoint(const QJsonValue& value, float* out)
{
    const QJsonArray a = value.toArray();
    for (int i = 0; i < 3; i++) {
        out[i] = static_cast<float>(a.at(i).toDouble());
    }
}

const ArkanModel& arkan()
{
    static const ArkanModel model = [] {
        ArkanModel m;
        QFile file(QStringLiteral(":/sculk_theme/arkan_model.json"));
        if (!file.open(QIODevice::ReadOnly)) {
            return m;
        }
        const QJsonObject json = QJsonDocument::fromJson(file.readAll()).object();
        const QJsonObject pivots = json.value("pivots").toObject();
        const auto partOf = [](const QJsonValue& name) {
            for (int i = 0; i < s_namedParts; i++) {
                if (name.toString() == QLatin1String(s_parts[i])) {
                    return i;
                }
            }
            return -1;
        };
        for (int i = 0; i < s_namedParts; i++) {
            readPoint(pivots.value(s_parts[i]), m.pivots[i]);
        }
        readPoint(json.value("hands").toObject().value("rarm"), m.hands[RightArm]);
        readPoint(json.value("hands").toObject().value("larm"), m.hands[LeftArm]);
        m.magic = json.value("magicHand").toString() == QLatin1String("larm") ? LeftArm : RightArm;
        for (const auto& value : json.value("glows").toArray()) {
            const QJsonObject g = value.toObject();
            const int part = partOf(g.value("part"));
            float at[3];
            readPoint(g.value("at"), at);
            if (part >= 0) {
                m.glows.push_back({ float(part), at[0], at[1], at[2], float(g.value("radius").toDouble(2)) });
            }
        }
        for (const auto& eye : json.value("eyes").toArray()) {
            std::array<float, 3> e{};
            readPoint(eye, e.data());
            m.eyes.push_back(e);
        }
        for (const auto& value : json.value("boxes").toArray()) {
            const QJsonObject b = value.toObject();
            int part = partOf(b.value("part"));
            if (part < 0) {
                continue;
            }
            float from[3], size[3];
            readPoint(b.value("from"), from);
            readPoint(b.value("size"), size);
            if (part == Body && from[1] < 1) {
                part = Skirt;  // the legs sway a little on their own, as he floats
            }
            const QJsonArray uv = b.value("uv").toArray();
            m.boxes.push_back({ float(part), from[0], from[1], from[2], size[0], size[1], size[2], float(uv.at(0).toDouble()),
                                float(uv.at(1).toDouble()), float(b.value("inflate").toDouble(0)) });
        }
        m.texture = QImage(QStringLiteral(":/sculk_theme/arkan.png")).convertToFormat(QImage::Format_ARGB32);
        if (m.texture.isNull()) {
            m.boxes.clear();  // no texture: the widget stays empty rather than drawing black boxes
        }
        return m;
    }();
    return model;
}

/* ------------------------------------------------------------ 3x4 matrices, as in the phone's CharacterView */

using Matrix = std::array<float, 12>;

Matrix mul(const Matrix& a, const Matrix& b)
{
    Matrix r{};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
            float v = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
            if (j == 3) {
                v += a[i * 4 + 3];
            }
            r[i * 4 + j] = v;
        }
    }
    return r;
}

/// a rotation about a pivot: first z, then x, then y
Matrix rot(float px, float py, float pz, float rx, float ry, float rz)
{
    const float cx = std::cos(rx), sx = std::sin(rx), cy = std::cos(ry), sy = std::sin(ry), cz = std::cos(rz), sz = std::sin(rz);
    const Matrix z{ cz, -sz, 0, 0, sz, cz, 0, 0, 0, 0, 1, 0 };
    const Matrix x{ 1, 0, 0, 0, 0, cx, -sx, 0, 0, sx, cx, 0 };
    const Matrix y{ cy, 0, sy, 0, 0, 1, 0, 0, -sy, 0, cy, 0 };
    Matrix r = mul(y, mul(x, z));
    r[3] = px - (r[0] * px + r[1] * py + r[2] * pz);
    r[7] = py - (r[4] * px + r[5] * py + r[6] * pz);
    r[11] = pz - (r[8] * px + r[9] * py + r[10] * pz);
    return r;
}

Matrix translate(float x, float y, float z)
{
    return { 1, 0, 0, x, 0, 1, 0, y, 0, 0, 1, z };
}

std::array<float, 3> transformPoint(const Matrix& m, const float* p)
{
    return { m[0] * p[0] + m[1] * p[1] + m[2] * p[2] + m[3], m[4] * p[0] + m[5] * p[1] + m[6] * p[2] + m[7],
             m[8] * p[0] + m[9] * p[1] + m[10] * p[2] + m[11] };
}

QColor withAlpha(QColor color, double alpha)
{
    color.setAlphaF(std::clamp(alpha, 0.0, 1.0));
    return color;
}

/// a soft round glow
void glowAt(QPainter& p, const QPointF& c, double radius, const QColor& color, double alpha)
{
    if (radius <= 0) {
        return;
    }
    QRadialGradient gradient(c, radius);
    gradient.setColorAt(0, withAlpha(color, alpha));
    gradient.setColorAt(1, withAlpha(color, 0));
    p.setPen(Qt::NoPen);
    p.setBrush(gradient);
    p.drawEllipse(c, radius, radius);
}
}  // namespace

MoonyWidget::MoonyWidget(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setMinimumSize(48, 48);
    m_timer.setInterval(40);  // 25 frames per second is plenty for a calm animation
    connect(&m_timer, &QTimer::timeout, this, &MoonyWidget::tick);
    m_clock.start();
}

void MoonyWidget::setExcited(bool excited)
{
    m_excited = excited;
}

void MoonyWidget::setBubbleVisible(bool visible)
{
    m_bubble = visible;
    update();
}

void MoonyWidget::setTalking(bool talking)
{
    m_talking = talking;
}

void MoonyWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    m_timer.start();
}

void MoonyWidget::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    m_timer.stop();  // no animation work while the launcher is hidden or minimized
}

void MoonyWidget::tick()
{
    const double now = m_clock.elapsed() / 1000.0;
    const double dt = std::clamp(now - m_lastFrame, 0.0, 0.1);
    m_lastFrame = now;
    m_time = now;
    // the eyes brighten and the work speeds up softly
    m_wake += ((m_excited || m_talking ? 1.0 : 0.0) - m_wake) * std::min(1.0, dt * 6.0);
    m_busy += ((m_talking ? 1.0 : 0.0) - m_busy) * std::min(1.0, dt * 4.0);
    m_phase += dt * (1.0 + 1.2 * m_busy);
    m_talk = m_talking ? 0.5 + 0.5 * std::sin(now * 11.0) : std::max(0.0, m_talk - dt * 4.0);
    update();
}

void MoonyWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    // with the bubble Arkan stands a little lower and to the left, the bubble above him on the right
    const int side = static_cast<int>(m_bubble ? std::min(width() * 0.78, height() * 0.86) : std::min(width(), height()));
    const QRect square(m_bubble ? 0 : (width() - side) / 2, height() - side, side, side);
    paintArkan(painter, m_bubble || side < 80 ? square : rect());
    if (m_bubble) {
        const double h = side * 0.3;
        const double w = h * 1.9;
        paintBubble(painter, QRectF(std::min(width() - w - 1.0, square.center().x() + side * 0.12), 1, w, h));
    }
}

QPointF MoonyWidget::project(const Matrix& m, float x, float y, float z, float* depth) const
{
    const float p[3] = { x, y, z };
    const auto q = transformPoint(m, p);
    if (depth) {
        *depth = q[2];
    }
    return { m_centerX + q[0] * m_scale, m_centerY - q[1] * m_scale };
}

void MoonyWidget::paintArkan(QPainter& p, const QRect& area)
{
    const ArkanModel& m = arkan();
    if (area.width() <= 0 || area.height() <= 0 || m.boxes.empty()) {
        return;
    }
    const bool closeUp = std::min(area.width(), area.height()) < 80;  // a small place: the head and shoulders
    // the frame in the screen's own pixels (Windows scaling included): sharp edges and even texture pixels
    const double density = devicePixelRatioF();
    const int bufW = std::max(1, static_cast<int>(std::lround(area.width() * density)));
    const int bufH = std::max(1, static_cast<int>(std::lround(area.height() * density)));
    const double k = 1.0 / density;  // frame pixels -> widget pixels
    if (m_frame.width() != bufW || m_frame.height() != bufH) {
        m_frame = QImage(bufW, bufH, QImage::Format_ARGB32_Premultiplied);
        m_depth.assign(static_cast<size_t>(bufW) * bufH, 0);
    }
    const float time = static_cast<float>(m_time), phase = static_cast<float>(m_phase), busy = static_cast<float>(m_busy);

    // the pose: floating, turning a little towards the raised hand, looking around, the hand up with the tesseract
    const int magic = m.magic, other = magic == RightArm ? LeftArm : RightArm;
    const float side = m.pivots[magic][0] > 0 ? 1.0f : -1.0f;  // +1: the raised hand is on the right of the picture
    const float bob = 2.2f + std::sin(time * 1.6f) * 0.7f;
    // turned a little towards the raised hand; in a close-up he looks at you
    const float yaw = closeUp ? -side * 0.12f + std::sin(time * 0.45f) * 0.12f : -side * 0.38f + std::sin(time * 0.45f) * 0.32f;
    const float headYaw = std::sin(time * 0.7f) * (closeUp ? 0.15f : 0.28f), headPitch = std::sin(time * 0.5f) * 0.08f - 0.04f;
    // the hand out to his side, the palm up, the tesseract over it (the face stays in sight)
    const float mPitch = -0.55f + std::sin(phase * 1.6f) * (0.05f + 0.13f * busy);
    const float mRoll = side * (1.2f + std::sin(phase * 1.1f) * (0.05f + 0.1f * busy));
    // the other hand hangs by his side
    const float oPitch = -0.12f + std::sin(time * 1.3f) * 0.05f, oRoll = -side * (0.08f + std::sin(time * 5.0f) * 0.05f * busy);
    const float skirtSway = std::sin(time * 1.6f + 1.0f) * 0.045f;
    m_blink = std::fmod(time, 4.7f) > 4.55f;

    Matrix camera = mul(rot(0, 0, 0, -0.16f, 0, 0), rot(0, 0, 0, 0, yaw, 0));
    const Matrix ground = camera;
    camera = mul(camera, translate(0, bob, 0));
    Matrix mats[PartCount];
    mats[Body] = camera;
    mats[Head] = mul(camera, rot(m.pivots[Head][0], m.pivots[Head][1], m.pivots[Head][2], headPitch, headYaw, 0));
    const Matrix magicArm = rot(m.pivots[magic][0], m.pivots[magic][1], m.pivots[magic][2], mPitch, 0, mRoll);
    mats[magic] = mul(camera, magicArm);
    mats[other] = mul(camera, rot(m.pivots[other][0], m.pivots[other][1], m.pivots[other][2], oPitch, 0, oRoll));
    mats[Skirt] = mul(camera, rot(0, 12, 0, skirtSway, 0, 0));
    // the cube over the raised palm; its violet light falls on the suit near it
    const double pulse = 0.75 + 0.25 * std::sin(time * (3.0 + 4.0 * busy));
    auto palm = transformPoint(magicArm, m.hands[magic]);
    palm[1] += 6.5f;
    m_tesseract = transformPoint(camera, palm.data());
    m_tesseractLight = static_cast<float>((0.8 + 0.2 * busy) * (0.85 + 0.15 * pulse));

    if (closeUp) {
        m_scale = std::min(bufW / 20.0f, bufH / 19.0f);
        m_centerX = bufW / 2.0f;
        m_centerY = bufH * 0.5f + (28 + bob) * m_scale;
    } else {
        m_scale = std::min(bufW / 38.0f, bufH / 46.0f);
        m_centerX = bufW * (0.5f - side * 0.05f);  // room for the raised hand and its tesseract
        m_centerY = bufH * 0.5f + 20 * m_scale;
    }
    m_frame.fill(Qt::transparent);
    std::fill(m_depth.begin(), m_depth.end(), -1e9f);

    // the hover ring under him: dots of light turning, violet outside, blue inside
    if (!closeUp) {
        const float turn = phase * 0.4f;
        for (int i = 0; i < 40; i++) {
            const float a = turn + i * s_pi * 2 / 40;
            runeDot(ground, std::cos(a) * 11, 0, std::sin(a) * 11, i % 5 == 0 ? 0xFFAB81FF : 0xFF8154F0);
        }
        for (int i = 0; i < 12; i++) {
            const float a = -turn * 1.5f + i * s_pi * 2 / 12;
            runeDot(ground, std::cos(a) * 7, 0, std::sin(a) * 7, 0xFF7CD0FF);
        }
    }
    // the model
    for (const auto& box : m.boxes) {
        drawBox(mats[static_cast<int>(box[0])], box.data(), static_cast<int>(box[0]) == Head);
    }

    // everything below is in widget pixels: the area's corner plus k widget pixels per frame pixel
    const auto at = [&](const QPointF& point) { return QPointF(area.left() + point.x() * k, area.top() + point.y() * k); };
    const double unit = m_scale * k;  // one model pixel in widget pixels
    p.save();
    p.setClipRect(area);
    p.setRenderHint(QPainter::Antialiasing, true);
    // the light under him, then the frame, sharp
    if (!closeUp) {
        const QPointF foot = at(project(ground, 0, 0, 0));
        p.save();
        p.translate(foot);
        p.scale(1, 0.3);
        glowAt(p, QPointF(0, 0), 15 * unit, QColor(0x81, 0x54, 0xf0), 0.5);
        p.restore();
    }
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    m_frame.setDevicePixelRatio(density);  // one frame pixel = one screen pixel
    p.drawImage(QPointF(area.topLeft()), m_frame);

    // the glow of the antenna light: brighter when he is awake or working
    if (!m_blink) {
        const double shine = 0.7 + 0.3 * std::sin(time * (2.2 + 3.8 * busy));
        for (const auto& e : m.eyes) {
            glowAt(p, at(project(mats[Head], e[0], e[1], e[2])), unit * 2.2, QColor(131, 72, 255), (0.4 + 0.25 * m_wake) * shine);
        }
    }
    // the screen on his chest glows
    for (const auto& g : m.glows) {
        glowAt(p, at(project(mats[static_cast<int>(g[0])], g[1], g[2], g[3])), unit * g[4] * (1.0 + 0.4 * busy) * (0.85 + 0.15 * std::sin(time * 3.0)),
               QColor(131, 72, 255), 0.44);
    }

    // the tesseract over the raised hand, turning in four dimensions
    const QPointF c = at(QPointF(m_centerX + m_tesseract[0] * m_scale, m_centerY - m_tesseract[1] * m_scale)) +
                      QPointF(0, std::sin(time * 2.0) * unit * 0.4);
    glowAt(p, c, unit * 7 * (1.0 + 0.3 * std::max<double>(busy, m_wake)), QColor(134, 72, 255), (0.3 + 0.15 * m_wake) * pulse);
    QPointF corners[16];
    const double size = unit * 15, turn = phase * 0.7;
    for (int i = 0; i < 16; i++) {
        const double x = (i & 1) ? 1 : -1, y = (i & 2) ? 1 : -1, z = (i & 4) ? 1 : -1, w = (i & 8) ? 1 : -1;
        const double a = turn, b = turn * 0.7;
        const double x1 = x * std::cos(a) - w * std::sin(a), w1 = x * std::sin(a) + w * std::cos(a);
        const double y1 = y * std::cos(b) - z * std::sin(b), z1 = y * std::sin(b) + z * std::cos(b);
        const double x2 = x1 * std::cos(0.5) - z1 * std::sin(0.5), z2 = x1 * std::sin(0.5) + z1 * std::cos(0.5);
        const double k4 = 1.0 / (3.0 - w1);
        const double k3 = 1.0 / (4.0 - z2 * k4);
        corners[i] = c + QPointF(x2 * k4 * k3 * size, y1 * k4 * k3 * size);
    }
    p.setPen(QPen(withAlpha(QColor(154, 98, 255), (200 + 55 * pulse) / 255.0), std::max(1.0, unit * 0.45), Qt::SolidLine, Qt::RoundCap));
    for (int i = 0; i < 16; i++) {
        for (int bit = 0; bit < 4; bit++) {
            const int j = i ^ (1 << bit);
            if (i < j) {
                p.drawLine(corners[i], corners[j]);
            }
        }
    }
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(224, 208, 255));
    const double dot = std::max(1.0, unit * 0.5);
    for (const auto& corner : corners) {
        p.drawEllipse(corner, dot, dot);
    }
    // sparks rise from the cube and fade: violet and blue
    const bool working = busy > 0.5f;
    const int count = working ? 12 : 7;
    const double period = working ? 1.4 : 2.4;
    for (int i = 0; i < count; i++) {
        const double t = std::fmod(time + i * period / count, period) / period;
        const double sx = c.x() + std::sin(i * 12.9 + t * 3) * unit * 4;
        const double sy = c.y() - unit * 2 - t * unit * 11;
        const double s = unit * (t < 0.5 ? 0.8 : 0.55);
        p.fillRect(QRectF(sx, sy, s, s), withAlpha(i % 3 == 0 ? QColor(124, 208, 255) : QColor(154, 98, 255), 1.0 - t));
    }
    p.restore();
}

/// one dot of the hover ring, with the depth test (his legs hide the back of the ring)
void MoonyWidget::runeDot(const Matrix& m, float x, float y, float z, QRgb color)
{
    float depth = 0;
    const QPointF point = project(m, x, y, z, &depth);
    const int size = std::max(1, static_cast<int>(std::lround(m_scale * 0.7f)));
    const int x0 = static_cast<int>(std::lround(point.x() - size / 2.0)), y0 = static_cast<int>(std::lround(point.y() - size / 2.0));
    const int bufW = m_frame.width(), bufH = m_frame.height();
    for (int j = 0; j < size; j++) {
        const int sy = y0 + j;
        if (sy < 0 || sy >= bufH) {
            continue;
        }
        auto* row = reinterpret_cast<QRgb*>(m_frame.scanLine(sy));
        for (int i = 0; i < size; i++) {
            const int sx = x0 + i;
            if (sx < 0 || sx >= bufW) {
                continue;
            }
            float& d = m_depth[static_cast<size_t>(sy) * bufW + sx];
            if (depth > d) {
                row[sx] = color;
                d = depth;
            }
        }
    }
}

/// a box with the Minecraft texture layout at (u, v): top and bottom on the first row, then right, front, left, back
void MoonyWidget::drawBox(const Matrix& m, const float* b, bool head)
{
    const float inflate = b[9];
    const float x0 = b[1] - inflate, x1 = b[1] + b[4] + inflate, y0 = b[2] - inflate, y1 = b[2] + b[5] + inflate;
    const float z0 = b[3] - inflate, z1 = b[3] + b[6] + inflate;
    const int u = static_cast<int>(b[7]), v = static_cast<int>(b[8]);
    const int iw = static_cast<int>(std::lround(b[4])), ih = static_cast<int>(std::lround(b[5])), id = static_cast<int>(std::lround(b[6]));
    const auto f = [&](float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz, int su, int sv, int sw, int sh) {
        const float a[3] = { ax, ay, az }, bb[3] = { bx, by, bz }, c[3] = { cx, cy, cz };
        face(m, a, bb, c, su, sv, sw, sh, head);
    };
    f(x0, y1, z1, x1, y1, z1, x0, y0, z1, u + id, v + id, iw, ih);
    f(x1, y1, z0, x0, y1, z0, x1, y0, z0, u + 2 * id + iw, v + id, iw, ih);
    f(x0, y1, z0, x0, y1, z1, x0, y0, z0, u, v + id, id, ih);
    f(x1, y1, z1, x1, y1, z0, x1, y0, z1, u + id + iw, v + id, id, ih);
    f(x0, y1, z0, x1, y1, z0, x0, y1, z1, u + id, v, iw, id);
    f(x0, y0, z1, x1, y0, z1, x0, y0, z0, u + id + iw, v, iw, id);
}

/// one face, a textured parallelogram, shaded like Minecraft and lit violet by the cube when it is near and turned to
/// it; the pixels with alpha 254 glow (not shaded), those of the head (the antenna light) go dark when he "blinks"
void MoonyWidget::face(const Matrix& m, const float* a, const float* b, const float* c, int su, int sv, int sw, int sh, bool head)
{
    const auto A = transformPoint(m, a), B = transformPoint(m, b), C = transformPoint(m, c);
    const float ex = B[0] - A[0], ey = B[1] - A[1], ez = B[2] - A[2], fx = C[0] - A[0], fy = C[1] - A[1], fz = C[2] - A[2];
    const float nx = fy * ez - fz * ey, ny = fz * ex - fx * ez, nz = fx * ey - fy * ex;
    const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
    if (len < 1e-6f || nz / len <= 0.001f) {
        return;  // turned away
    }
    const float light = (nx * -0.35f + ny * 0.8f + nz * 0.5f) / len / 0.995f;
    float bright = 0.56f + 0.46f * std::max(0.0f, light);
    if (ny / len < -0.5f) {
        bright *= 0.85f;
    }
    // the violet light of the cube: more blue than red
    const float dx = m_tesseract[0] - (A[0] + (ex + fx) / 2), dy = m_tesseract[1] - (A[1] + (ey + fy) / 2),
                dz = m_tesseract[2] - (A[2] + (ez + fz) / 2);
    const float dist = std::sqrt(dx * dx + dy * dy + dz * dz) + 1e-3f;
    const float lit = std::max(0.0f, (nx * dx + ny * dy + nz * dz) / (len * dist)) * std::clamp(1.1f - dist / 13, 0.0f, 1.0f) * m_tesseractLight;
    const int b256 = static_cast<int>(std::lround(std::min(1.08f, bright) * 256));
    const int r256 = static_cast<int>(std::lround(std::min(1.3f, bright + 0.35f * lit) * 256));
    const int v256 = static_cast<int>(std::lround(std::min(1.4f, bright + 0.7f * lit) * 256));
    const int add = static_cast<int>(std::lround(lit * 22));
    const float sAx = m_centerX + A[0] * m_scale, sAy = m_centerY - A[1] * m_scale;
    const float e1x = ex * m_scale, e1y = -ey * m_scale, e2x = fx * m_scale, e2y = -fy * m_scale;
    const float det = e1x * e2y - e1y * e2x;
    if (std::abs(det) < 1e-4f) {
        return;
    }
    const int bufW = m_frame.width(), bufH = m_frame.height();
    const float minX = std::min({ sAx, sAx + e1x, sAx + e2x, sAx + e1x + e2x }), maxX = std::max({ sAx, sAx + e1x, sAx + e2x, sAx + e1x + e2x });
    const float minY = std::min({ sAy, sAy + e1y, sAy + e2y, sAy + e1y + e2y }), maxY = std::max({ sAy, sAy + e1y, sAy + e2y, sAy + e1y + e2y });
    const int x0 = std::max(0, static_cast<int>(std::floor(minX))), x1 = std::min(bufW - 1, static_cast<int>(std::ceil(maxX)));
    const int y0 = std::max(0, static_cast<int>(std::floor(minY))), y1 = std::min(bufH - 1, static_cast<int>(std::ceil(maxY)));
    const float dsdx = e2y / det, dsdy = -e2x / det, dtdx = -e1y / det, dtdy = e1x / det;
    const float dz1 = B[2] - A[2], dz2 = C[2] - A[2];
    const QImage& texture = arkan().texture;
    const int tw = texture.width(), th = texture.height();
    for (int py = y0; py <= y1; py++) {
        const float ry = py + 0.5f - sAy, rx = x0 + 0.5f - sAx;
        float s = rx * dsdx + ry * dsdy, t = rx * dtdx + ry * dtdy;
        auto* row = reinterpret_cast<QRgb*>(m_frame.scanLine(py));
        float* depthRow = m_depth.data() + static_cast<size_t>(py) * bufW;
        for (int px = x0; px <= x1; px++, s += dsdx, t += dtdx) {
            if (s < -0.02f || s >= 1.02f || t < -0.02f || t >= 1.02f) {
                continue;
            }
            const float z = A[2] + s * dz1 + t * dz2;
            if (z <= depthRow[px]) {
                continue;
            }
            const int ux = su + static_cast<int>(std::clamp(s, 0.0f, 0.999f) * sw), vy = sv + static_cast<int>(std::clamp(t, 0.0f, 0.999f) * sh);
            if (ux < 0 || vy < 0 || ux >= tw || vy >= th) {
                continue;
            }
            const QRgb texel = reinterpret_cast<const QRgb*>(texture.constScanLine(vy))[ux];
            const int alpha = qAlpha(texel);
            if (alpha < 128) {
                continue;
            }
            if (alpha == 254) {
                // a glowing pixel: the antenna light (dark for a moment now and then), the screen
                row[px] = m_blink && head ? 0xFF040512 : (0xFF000000 | (texel & 0xFFFFFF));
            } else {
                row[px] = qRgb(std::min(255, ((qRed(texel) * r256) >> 8) + add / 2), std::min(255, ((qGreen(texel) * b256) >> 8) + add / 8),
                               std::min(255, ((qBlue(texel) * v256) >> 8) + add));
            }
            depthRow[px] = z;
        }
    }
}

void MoonyWidget::paintBubble(QPainter& painter, const QRectF& box)
{
    const double w = box.width();
    const double h = box.height();
    QPainterPath path;
    path.addRoundedRect(box, h * 0.35, h * 0.35);
    QPolygonF tail;
    tail << QPointF(box.left() + w * 0.18, box.bottom() - 1) << QPointF(box.left() + w * 0.02, box.bottom() + h * 0.4)
         << QPointF(box.left() + w * 0.38, box.bottom() - 1);
    path.addPolygon(tail);
    path = path.simplified();
    painter.setPen(QPen(QColor(159, 115, 255, 230), std::max(1.5, w * 0.045)));
    painter.setBrush(QColor(0x0a, 0x08, 0x1c));
    painter.drawPath(path);

    // three dots in violet shades
    const QColor dots[] = { QColor(159, 115, 255), QColor(129, 84, 240), QColor(189, 157, 255) };
    const double r = h * 0.12;
    for (int i = 0; i < 3; i++) {
        double lift = 0;
        if (m_talking) {
            // the dots jump one after another, like someone typing
            lift = std::max(0.0, std::sin(m_time * 7.0 - i * 0.9)) * h * 0.18;
        }
        painter.setPen(Qt::NoPen);
        painter.setBrush(dots[i]);
        painter.drawEllipse(QPointF(box.left() + w * (0.28 + i * 0.22), box.center().y() - lift), r, r);
    }
}
