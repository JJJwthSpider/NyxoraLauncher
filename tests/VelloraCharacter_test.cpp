// SPDX-License-Identifier: GPL-3.0-only
// The 3D player of the lobby (ui/widgets/VelloraCharacter) is drawn in the screen's own pixels, Windows scaling included:
// a frame drawn smaller and stretched made the skin look made of small square pixels with stepped edges on big screens.
// Set VELLORA_SHOTS to a folder to save the pictures.

#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QDir>
#include <QImage>
#include <QSet>
#include <QTest>

#include "ui/themes/VelloraLook.h"
#include "ui/widgets/VelloraCharacter.h"
#include "ui/widgets/VelloraSkinGen.h"

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

    /// the look for the game (the Vellora Cosmetics mod): boxes on the parts of the Minecraft player model
    void exportLook()
    {
        QTemporaryDir dir;
        QImage legacySkin(64, 32, QImage::Format_ARGB32);
        legacySkin.fill(qRgb(120, 80, 60));
        QImage cape(64, 32, QImage::Format_ARGB32);
        cape.fill(qRgb(150, 20, 40));
        QVERIFY(VelloraCharacter::exportLook(dir.path(), legacySkin, true, cape, "crown", "chain", "angel_wings"));

        QFile file(dir.filePath("look.json"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QJsonObject look = QJsonDocument::fromJson(file.readAll()).object();
        QCOMPARE(look.value("skin").toString(), QString("skin.png"));
        QCOMPARE(look.value("cape").toString(), QString("cape.png"));
        QVERIFY(look.value("slim").toBool());
        QCOMPARE(QImage(dir.filePath("skin.png")).size(), QSize(64, 64));  // the old 64x32 skin in the 64x64 layout
        QCOMPARE(QImage(dir.filePath("cape.png")).size(), QSize(64, 32));

        const QJsonObject parts = look.value("parts").toObject();
        const auto boxes = [&parts](const char* part) { return parts.value(part).toArray(); };
        QVERIFY2(!boxes("head").isEmpty(), "the crown is on the head");
        QVERIFY2(!boxes("body").isEmpty(), "the chain and the wings are on the body");
        // Minecraft's head: y from -8 (the top) to 0 (the neck), the face at z -4: the crown is above the top
        double crownTop = 0;
        for (const auto& b : boxes("head")) {
            crownTop = std::min(crownTop, b.toArray().at(1).toDouble());
        }
        QVERIFY2(crownTop < -8, "the crown is above the head");
        // the body: z from -2 (the front) to 2 (the back): the chain in front, the wings behind
        int front = 0, behind = 0;
        for (const auto& value : boxes("body")) {
            const QJsonArray b = value.toArray();
            front += b.at(5).toDouble() <= -2 ? 1 : 0;
            behind += b.at(2).toDouble() >= 2 ? 1 : 0;
            QVERIFY(b.at(0).toDouble() < b.at(3).toDouble() && b.at(1).toDouble() < b.at(4).toDouble() && b.at(2).toDouble() < b.at(5).toDouble());
            QVERIFY(qAlpha(QRgb(quint32(b.at(6).toDouble()))) > 0);
        }
        QVERIFY2(front > 0, "the chain is on the front of the body");
        QVERIFY2(behind > 0, "the wings are on the back");

        // VELLORA_LOOK: the same look with a skin of the generator, for a look at it in the game (cosmetics-mod/run)
        const QString keep = qEnvironmentVariable("VELLORA_LOOK");
        if (!keep.isEmpty()) {
            const auto skins = SkinGen::skins("knight", 1, 3);
            QImage crimsonCape(64, 32, QImage::Format_ARGB32);
            crimsonCape.fill(Qt::transparent);
            for (int y = 0; y < 17; y++) {
                for (int x = 0; x < 22; x++) {
                    crimsonCape.setPixel(x, y, (x + y) % 5 == 0 ? qRgb(240, 200, 110) : qRgb(140 + (y * 3), 16, 36));
                }
            }
            QVERIFY(VelloraCharacter::exportLook(keep, skins.first().image, skins.first().slim, crimsonCape, "crown", "chain", "angel_wings"));
        }
    }

    /// the mod is made for Fabric and Quilt on Minecraft 1.20.1: anywhere else Fabric would not start with it
    void lookFits()
    {
        QVERIFY(VelloraLook::fits({ { "net.minecraft", "1.20.1" }, { "net.fabricmc.fabric-loader", "0.16.14" } }));
        QVERIFY(VelloraLook::fits({ { "net.minecraft", "1.20.1" }, { "org.quiltmc.quilt-loader", "0.26.0" } }));
        QVERIFY(!VelloraLook::fits({ { "net.minecraft", "1.20.1" }, { "net.minecraftforge", "47.3.0" } }));
        QVERIFY(!VelloraLook::fits({ { "net.minecraft", "1.21.1" }, { "net.fabricmc.fabric-loader", "0.16.14" } }));
        QVERIFY(!VelloraLook::fits({ { "net.minecraft", "1.20.1" } }));
    }

    /// nothing worn and no skin: an empty look, the game keeps its own
    void exportNothing()
    {
        QTemporaryDir dir;
        QVERIFY(VelloraCharacter::exportLook(dir.path(), QImage(), false, QImage(), QString(), QString(), QString()));
        QFile file(dir.filePath("look.json"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QJsonObject look = QJsonDocument::fromJson(file.readAll()).object();
        QVERIFY(!look.contains("skin") && !look.contains("cape"));
        QVERIFY(look.value("parts").toObject().isEmpty());
        QVERIFY(!QFile::exists(dir.filePath("skin.png")));
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
