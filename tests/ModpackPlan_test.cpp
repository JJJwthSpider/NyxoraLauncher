// SPDX-License-Identifier: GPL-3.0-only
// What Arkan, the pack builder of Vellora Client, understands in a request (launcher/modplatform/agent/ModpackPlan.cpp)

#include <QTest>

#include "modplatform/agent/ModpackPlan.h"

using namespace ModpackAgent;

class ModpackPlanTest : public QObject {
    Q_OBJECT

    static bool hasTheme(const Plan& plan, const QString& query)
    {
        for (const auto& theme : plan.themes) {
            if (theme.queries.contains(query)) {
                return true;
            }
        }
        return false;
    }

    static bool hasCurated(const Plan& plan, const QString& slug)
    {
        for (const auto& mod : plan.curated) {
            if (mod.alternatives.contains(slug)) {
                return true;
            }
        }
        return false;
    }

   private slots:
    void versionAndLoader()
    {
        const auto plan = parseLocally(QStringLiteral("сборка про магию на fabric 1.20.1"));
        QCOMPARE(plan.minecraftVersion, QStringLiteral("1.20.1"));
        QCOMPARE(plan.loader, QStringLiteral("fabric"));
        QVERIFY(hasTheme(plan, "magic"));
    }

    void typos()
    {
        // "зомьби" and "драконнов" are zombies and dragons
        const auto plan = parseLocally(QStringLiteral("сборка про зомьби и драконнов"));
        QVERIFY(hasTheme(plan, "zombie"));
        QVERIFY(hasTheme(plan, "dragons"));
    }

    void weakComputer()
    {
        const auto plan = parseLocally(QStringLiteral("приключения и шейдеры для слабого пк"));
        QVERIFY(plan.lowEnd);
        QVERIFY(!hasCurated(plan, "iris"));  // no shaders on a weak computer
        for (const auto& theme : plan.themes) {
            QVERIFY(theme.count <= 2);
        }
        QVERIFY(!plan.notes.isEmpty());
    }

    void friends()
    {
        const auto plan = parseLocally(QStringLiteral("хочу играть с друзьями, технологии"));
        QVERIFY(plan.multiplayer);
        QVERIFY(hasCurated(plan, "simple-voice-chat"));
    }

    void numberOfMods()
    {
        const auto plan = parseLocally(QStringLiteral("магия и технологии, 40 модов"));
        QCOMPARE(plan.targetMods, 40);
        int themed = 0;
        for (const auto& theme : plan.themes) {
            themed += theme.count;
        }
        QVERIFY(themed > 10);
    }

    void optifine()
    {
        const auto plan = parseLocally(QStringLiteral("покемоны и динозавры, добавь OptiFine"));
        QVERIFY(!plan.requestedMods.contains("OptiFine", Qt::CaseInsensitive));
        QVERIFY(hasCurated(plan, "iris"));
        QVERIFY(hasTheme(plan, "cobblemon"));
        QVERIFY(hasTheme(plan, "dinosaurs"));
    }

    void without()
    {
        const auto plan = parseLocally(QStringLiteral("зомби и оружие, без JEI"));
        QVERIFY(hasTheme(plan, "zombie"));
        QVERIFY(plan.excluded.contains("JEI", Qt::CaseInsensitive));
    }
};

QTEST_GUILESS_MAIN(ModpackPlanTest)

#include "ModpackPlan_test.moc"
