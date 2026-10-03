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

#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QTest>

#include <offline/GuideLinks.h>

using namespace OfflineGuide;

class GuideLinksTest : public QObject {
    Q_OBJECT
   private slots:
    void anchorsAreUniqueAndSimple()
    {
        QSet<QString> seen;
        const QRegularExpression simple("^[a-z][a-z0-9-]*$");
        for (auto section : allSections()) {
            const QString a = anchor(section);
            QVERIFY2(simple.match(a).hasMatch(), qPrintable(a));
            QVERIFY2(!seen.contains(a), qPrintable(a));
            seen.insert(a);
            QVERIFY(!title(section).isEmpty());
            QVERIFY(sectionFromAnchor(a) == section);
        }
        QCOMPARE(seen.size(), 16);
        QVERIFY(!sectionFromAnchor("nope"));
    }

    // Plan 6 replaces Guide.html with the real guide; this keeps every "?" button pointing at an existing section.
    void guideHasEveryAnchor()
    {
        QFile guide(QStringLiteral(PINECONE_GUIDE_HTML));
        QVERIFY2(guide.open(QIODevice::ReadOnly), PINECONE_GUIDE_HTML);
        const QString html = QString::fromUtf8(guide.readAll());
        for (auto section : allSections())
            QVERIFY2(html.contains(QString("id=\"%1\"").arg(anchor(section))), qPrintable(anchor(section)));
    }

    void sectionUrlKeepsTheAnchor()
    {
        const QUrl url = sectionUrl("C:/Games/PineconeMC Offline/Guide.html", Section::Lan);
        QCOMPARE(url.toLocalFile(), QString("C:/Games/PineconeMC Offline/Guide.html"));
        QCOMPARE(url.fragment(), QString("lan"));
    }

    void redirectPageForwardsWithTheAnchor()
    {
        const QString page = redirectPage(sectionUrl("C:/Games/PineconeMC Offline/Guide.html", Section::Lan));
        QVERIFY(page.contains("http-equiv=\"refresh\""));
        QVERIFY(page.contains("url=file:///C:/Games/PineconeMC%20Offline/Guide.html#lan"));
        QVERIFY(page.contains("href=\"file:///C:/Games/PineconeMC%20Offline/Guide.html#lan\""));
    }
};

QTEST_GUILESS_MAIN(GuideLinksTest)

#include "GuideLinks_test.moc"
