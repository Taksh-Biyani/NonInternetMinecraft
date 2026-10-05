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
#include <QTextBlock>
#include <QTextDocument>

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

    // Keeps every "?" button pointing at an existing section when Guide.html is edited.
    void guideHasEveryAnchor()
    {
        QFile guide(QStringLiteral(PINECONE_GUIDE_HTML));
        QVERIFY2(guide.open(QIODevice::ReadOnly), PINECONE_GUIDE_HTML);
        const QString html = QString::fromUtf8(guide.readAll());
        for (auto section : allSections())
            QVERIFY2(html.contains(QString("id=\"%1\"").arg(anchor(section))), qPrintable(anchor(section)));
    }

    // The in-app guide window renders the guide with Qt's rich text: every "?" button must still find its section there.
    void inAppGuideKeepsEveryAnchor()
    {
        QFile guide(QStringLiteral(PINECONE_GUIDE_HTML));
        QVERIFY(guide.open(QIODevice::ReadOnly));
        const QString html = forTextBrowser(QString::fromUtf8(guide.readAll()));
        QVERIFY(!html.contains("<style"));
        QVERIFY(!html.contains("<details"));
        QVERIFY(!html.contains("<summary"));

        QTextDocument doc;
        doc.setHtml(html);
        QSet<QString> names;
        for (QTextBlock block = doc.begin(); block.isValid(); block = block.next())
            for (auto it = block.begin(); !it.atEnd(); ++it)
                for (const QString& name : it.fragment().charFormat().anchorNames())
                    names.insert(name);
        for (auto section : allSections())
            QVERIFY2(names.contains(anchor(section)), qPrintable(anchor(section)));
    }

    void questionsBecomeHeadings()
    {
        const QString html = forTextBrowser("<details><summary>\"Damaged…\"</summary>\n<p>Copy it again.</p></details>");
        QCOMPARE(html, QString("<h4>\"Damaged…\"</h4>\n<p>Copy it again.</p>"));
    }

    void bundleDiagramBecomesOneRow()
    {
        const QString html = forTextBrowser(
            "<div class=\"flow\" role=\"img\">\n  <div class=\"step\">A</div>\n  <div class=\"arrow\" aria-hidden=\"true\">&rarr;</div>\n"
            "  <div class=\"step\">B</div>\n</div>\n<p>after</p>");
        QVERIFY2(html.contains("<table class=\"flow\""), qPrintable(html));
        QVERIFY(html.contains("<td class=\"step\">A</td>"));
        QVERIFY(html.contains("<td class=\"arrow\">&rarr;</td>"));
        QVERIFY(html.contains("</tr></table>\n<p>after</p>"));
        QVERIFY(!html.contains("<div"));
    }
};

QTEST_MAIN(GuideLinksTest)

#include "GuideLinks_test.moc"
