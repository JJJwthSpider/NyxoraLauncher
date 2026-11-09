// SPDX-License-Identifier: GPL-3.0-only
// The 3D player of the lobby (ui/widgets/VelloraCharacter) is drawn in the screen's own pixels, Windows scaling included:
// a frame drawn smaller and stretched made the skin look made of small square pixels with stepped edges on big screens.
// Set VELLORA_SHOTS to a folder to save the pictures.

#include <QApplication>
#include <QDir>
#include <QImage>
#include <QSet>
#include <QTest>

#include "ui/widgets/VelloraCharacter.h"

namespace {
/// the player at this size, drawn into an image of the screen's pixels (no background)
QImage shot(int w, int h, const QString& name)
{
    VelloraCharacter player;
    player.setAnimated(false);
    player.setShowItems(false);  // the accessories are read from the launcher's settings, there is none here
    // a skin of checked pixels (the edges of every skin pixel are seen), the second layer empty
    QImage skin(64, 64, QImage::Format_ARGB32);
    skin.fill(Qt::transparent);
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            skin.setPixel(x, y, (x + y) % 2 ? qRgb(200, 120, 90) : qRgb(70, 40, 120));
        }
    }
    for (int y = 48; y < 64; y++) {
        for (int x = 16; x < 48; x++) {
            skin.setPixel(x, y, (x + y) % 2 ? qRgb(60, 90, 160) : qRgb(30, 30, 40));
        }
    }
    player.setSkin(skin);
    player.resize(w, h);
    player.show();
    QTest::qWait(100);
    const qreal dpr = player.devicePixelRatioF();
    QImage image(qRound(w * dpr), qRound(h * dpr), QImage::Format_ARGB32);
    image.setDevicePixelRatio(dpr);
    image.fill(Qt::transparent);
    player.render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
    const QString folder = qEnvironmentVariable("VELLORA_SHOTS");
    if (!folder.isEmpty()) {
        image.save(QDir(folder).filePath(name));
    }
    return image;
}

/// where the player's silhouette starts on each row (the opaque pixels; the glow on the floor is see-through)
QList<int> leftEdges(const QImage& image)
{
    QList<int> edges;
    for (int y = 0; y < image.height(); y++) {
        for (int x = 0; x < image.width(); x++) {
            if (qAlpha(image.pixel(x, y)) == 255) {
                edges.append(x);
                break;
            }
        }
    }
    return edges;
}
}  // namespace

class VelloraCharacterTest : public QObject {
    Q_OBJECT
   private slots:
    void drawnInScreenPixels()
    {
        // a big lobby on a screen scaled to 200%: the old frame was 4x4 screen pixels a dot
        const QImage image = shot(450, 700, "player_big.png");  // 900x1400 screen pixels: drawn exactly in them
        QCOMPARE(image.devicePixelRatio(), 2.0);
        const QList<int> edges = leftEdges(image);
        QVERIFY2(edges.size() > image.height() / 2, "the player is drawn");
        QSet<int> phases;
        for (int x : edges) {
            phases.insert(x % 4);
        }
        // a stretched frame puts every edge on the same place of its 4x4 dot; drawn in screen pixels, edges fall anywhere
        QVERIFY2(phases.size() >= 3, "the edges fall only on the stretched dots: the frame is not in screen pixels");
    }
};

int main(int argc, char* argv[])
{
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
    qputenv("QT_SCALE_FACTOR", "2");  // a screen scaled to 200%
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(sculk_theme);
    VelloraCharacterTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "VelloraCharacter_test.moc"
