// SPDX-License-Identifier: GPL-3.0-only
// Arkan, the 3D wizard of the launcher (ui/widgets/MoonyWidget): his model and texture load from the resources and he is
// drawn in every place the launcher shows him: the full figure (the lobby banner), the head and shoulders (small places)
// and next to a speech bubble (the agent's chat). Set VELLORA_SHOTS to a folder to save the pictures.

#include <QApplication>
#include <QDir>
#include <QImage>
#include <QTest>

#include "ui/widgets/MoonyWidget.h"

namespace {
struct Counts {
    int opaque = 0;    // the model's pixels
    int glowing = 0;   // bright red: the eyes, the rubies, the tesseract, the magic circle
    int beard = 0;     // light grey
    int lowestRow = 0; // the lowest row with a model pixel
};

Counts look(const QImage& image)
{
    Counts c;
    for (int y = 0; y < image.height(); y++) {
        for (int x = 0; x < image.width(); x++) {
            const QRgb p = image.pixel(x, y);
            if (qAlpha(p) == 255) {
                c.opaque++;
                c.lowestRow = y;
                const int r = qRed(p), g = qGreen(p), b = qBlue(p);
                if (b > 200 && g < 150 && r > 110) {  // the bright violet of the eyes, the gems and the magic circle
                    c.glowing++;
                }
                if (r > 150 && std::abs(r - g) < 14 && std::abs(g - b) < 14) {
                    c.beard++;
                }
            }
        }
    }
    return c;
}

QImage shot(MoonyWidget& widget, const QString& name)
{
    widget.show();
    QTest::qWait(250);  // a few frames of the animation
    // in the screen's own pixels: with QT_SCALE_FACTOR=8 the 64x64 close-up is the 512x512 avatar (wizard.png)
    QImage image(widget.size() * widget.devicePixelRatioF(), QImage::Format_ARGB32);
    image.setDevicePixelRatio(widget.devicePixelRatioF());
    image.fill(Qt::transparent);
    widget.render(&image, QPoint(), QRegion(), QWidget::DrawChildren);  // without a background: only Arkan
    const QString folder = qEnvironmentVariable("VELLORA_SHOTS");
    if (!folder.isEmpty()) {
        image.save(QDir(folder).filePath(name));
    }
    return image;
}
}  // namespace

class MoonyWidgetTest : public QObject {
    Q_OBJECT
   private slots:
    void fullFigure()
    {
        MoonyWidget widget;
        widget.setExcited(true);
        widget.resize(116, 136);  // the lobby banner
        const Counts c = look(shot(widget, "arkan_banner.png"));
        QVERIFY2(c.opaque > 116 * 136 / 8, "the model is drawn");
        QVERIFY2(c.glowing > 20, "the eyes, the gems and the magic circle glow");
        QVERIFY2(c.beard > 20, "the silver beard is there");
        QVERIFY2(c.lowestRow > 136 * 3 / 4, "the whole figure, down to the magic circle");
    }

    void closeUp()
    {
        MoonyWidget widget;
        widget.resize(64, 64);  // a small place: only the head and shoulders
        const Counts c = look(shot(widget, "arkan_small.png"));
        QVERIFY2(c.opaque > 64 * 64 / 3, "the head fills the small place");
        QVERIFY2(c.beard > 40, "the beard is big in the close-up");
    }

    void withBubble()
    {
        MoonyWidget widget;
        widget.setBubbleVisible(true);
        widget.setTalking(true);  // the agent is working
        widget.resize(100, 70);   // the agent's chat header
        const QImage image = shot(widget, "arkan_chat.png");
        QVERIFY2(look(image).opaque > 400, "Arkan and the bubble are drawn");
        // the bubble is above his head on the right: a dark box with a violet rim over the empty top corner
        int bubble = 0;
        for (int y = 0; y < 12; y++) {
            for (int x = 50; x < 70; x++) {
                bubble += qAlpha(image.pixel(x, y)) == 255 && qRed(image.pixel(x, y)) < 60 ? 1 : 0;
            }
        }
        QVERIFY2(bubble > 40, "the bubble");
    }
};

int main(int argc, char* argv[])
{
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(sculk_theme);
    MoonyWidgetTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "MoonyWidget_test.moc"
