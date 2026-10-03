// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PineconeMC Offline - Minecraft Launcher
 *  Copyright (C) 2026 PineconeMC Offline Contributors
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

#include <QFont>
#include <QTest>

#include <ui/themes/Accessibility.h>
#include <ui/themes/HighContrastTheme.h>

using namespace Accessibility;

class AccessibilityTest : public QObject {
    Q_OBJECT
   private slots:
    void picksTheTheme()
    {
        QCOMPARE(effectiveTheme(Contrast::Off, "dark", true, false), QString("dark"));
        QCOMPARE(effectiveTheme(Contrast::Dark, "bright", false, false), HighContrastDarkTheme);
        QCOMPARE(effectiveTheme(Contrast::Light, "dark", false, false), HighContrastLightTheme);
        QCOMPARE(effectiveTheme(Contrast::Auto, "dark", false, false), QString("dark"));
        QCOMPARE(effectiveTheme(Contrast::Auto, "dark", true, false), HighContrastDarkTheme);
        QCOMPARE(effectiveTheme(Contrast::Auto, "dark", true, true), HighContrastLightTheme);
        QCOMPARE(contrastFromString("whatever"), Contrast::Auto);
        for (auto c : { Contrast::Auto, Contrast::Off, Contrast::Dark, Contrast::Light })
            QCOMPARE(contrastFromString(contrastToString(c)), c);
    }

    void scalesText()
    {
        QCOMPARE(clampTextScale(100), 100);
        QCOMPARE(clampTextScale(130), 125);
        QCOMPARE(clampTextScale(1000), 200);
        QCOMPARE(clampTextScale(0), 100);
        QFont base;
        base.setPointSizeF(9.0);
        QCOMPARE(scaledFont(base, 150).pointSizeF(), 13.5);
        QCOMPARE(scaledFont(base, 100).pointSizeF(), 9.0);
    }

    void measuresContrast()
    {
        QVERIFY(qAbs(contrastRatio(Qt::black, Qt::white) - 21.0) < 0.01);
        QVERIFY(qAbs(contrastRatio(Qt::white, Qt::white) - 1.0) < 0.01);
    }

    // WCAG AAA (7:1) for text, buttons, selection and links; disabled text still at least 4.5:1.
    void highContrastThemesAreHighContrast()
    {
        for (bool dark : { true, false }) {
            HighContrastTheme theme(dark);
            const QPalette p = theme.colorScheme();
            auto ratio = [&p](QPalette::ColorGroup g, QPalette::ColorRole fg, QPalette::ColorRole bg) {
                return contrastRatio(p.color(g, fg), p.color(g, bg));
            };
            QVERIFY(ratio(QPalette::Active, QPalette::WindowText, QPalette::Window) >= 7.0);
            QVERIFY(ratio(QPalette::Active, QPalette::Text, QPalette::Base) >= 7.0);
            QVERIFY(ratio(QPalette::Active, QPalette::ButtonText, QPalette::Button) >= 7.0);
            QVERIFY(ratio(QPalette::Active, QPalette::HighlightedText, QPalette::Highlight) >= 7.0);
            QVERIFY(ratio(QPalette::Active, QPalette::Link, QPalette::Base) >= 7.0);
            QVERIFY(ratio(QPalette::Disabled, QPalette::Text, QPalette::Base) >= 4.5);
            QVERIFY(ratio(QPalette::Inactive, QPalette::Text, QPalette::Base) >= 7.0);
            QVERIFY(!theme.appStyleSheet().isEmpty());
        }
    }
};

QTEST_MAIN(AccessibilityTest)

#include "Accessibility_test.moc"
