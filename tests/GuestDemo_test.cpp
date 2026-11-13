// SPDX-License-Identifier: GPL-3.0-only
// A guest of Nyxora plays the official Minecraft demo. The demo came in Minecraft 1.3.1: older versions ignore "--demo"
// and would run the full game, so they must get no demo (ui/themes/VelloraGuest).

#include <QTest>

#include "ui/themes/VelloraGuest.h"

class GuestDemoTest : public QObject {
    Q_OBJECT
   private slots:
    void versionsWithDemo_data()
    {
        QTest::addColumn<QString>("version");
        QTest::addColumn<bool>("demo");
        QTest::newRow("1.20.1") << "1.20.1" << true;
        QTest::newRow("1.21") << "1.21" << true;
        QTest::newRow("1.3.1, the first with the demo") << "1.3.1" << true;
        QTest::newRow("1.3.2") << "1.3.2" << true;
        QTest::newRow("a pre-release") << "1.20.1-pre1" << true;
        QTest::newRow("a release candidate") << "1.21-rc1" << true;
        QTest::newRow("a new snapshot") << "24w14a" << true;
        QTest::newRow("a 2013 snapshot") << "13w16a" << true;
        QTest::newRow("year-based") << "26.1" << true;
        QTest::newRow("1.3 (never released)") << "1.3" << false;
        QTest::newRow("1.2.5") << "1.2.5" << false;
        QTest::newRow("1.0") << "1.0" << false;
        QTest::newRow("beta") << "b1.7.3" << false;
        QTest::newRow("alpha") << "a1.2.6" << false;
        QTest::newRow("classic") << "c0.30_01c" << false;
        QTest::newRow("pre-classic") << "rd-132211" << false;
        QTest::newRow("infdev") << "inf-20100618" << false;
        QTest::newRow("a 2012 snapshot") << "12w30a" << false;
        QTest::newRow("nothing") << "" << false;
    }
    void versionsWithDemo()
    {
        QFETCH(QString, version);
        QFETCH(bool, demo);
        QCOMPARE(VelloraGuest::demoWorks(version), demo);
    }
};

QTEST_GUILESS_MAIN(GuestDemoTest)

#include "GuestDemo_test.moc"
