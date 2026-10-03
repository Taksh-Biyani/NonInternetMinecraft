// SPDX-License-Identifier: GPL-3.0-only

#include <QTest>

#include <offline/JavaChoice.h>

class JavaChoiceTest : public QObject {
    Q_OBJECT
   private slots:
    void prefersTheLaunchersOwnJava()
    {
        const QList<JavaChoice::Candidate> javas = { { "C:/Program Files/Java/jdk-25/bin/javaw.exe", 25 },
                                                     { "D:/Games/PineconeMC/java/temurin-25-jre/bin/javaw.exe", 25 } };
        QCOMPARE(JavaChoice::pick(javas, { 25 }, "D:/Games/PineconeMC/java"), 1);
    }
    void fallsBackToAnyCompatibleJava()
    {
        const QList<JavaChoice::Candidate> javas = { { "D:/Games/PineconeMC/java/temurin-8-jre/bin/javaw.exe", 8 },
                                                     { "C:/Program Files/Java/jdk-21/bin/javaw.exe", 21 } };
        QCOMPARE(JavaChoice::pick(javas, { 21 }, "D:/Games/PineconeMC/java"), 1);
    }
    void noneCompatible() { QCOMPARE(JavaChoice::pick({ { "C:/j/bin/javaw.exe", 8 } }, { 25 }, "D:/x/java"), -1); }
    void matchesFoldersNotPrefixes()
    {
        // "java-old" starts with "java" but is not inside the java folder.
        const QList<JavaChoice::Candidate> javas = { { "D:/x/java-old/jre/bin/javaw.exe", 17 },
                                                     { "D:/x/java/temurin-17-jre/bin/javaw.exe", 17 } };
        QCOMPARE(JavaChoice::pick(javas, { 17 }, "D:/x/java"), 1);
    }
    void ignoresCaseAndSeparators()
    {
        QCOMPARE(JavaChoice::pick({ { "c:\\Other\\bin\\javaw.exe", 17 }, { "d:\\X\\Java\\t\\bin\\javaw.exe", 17 } }, { 17 }, "D:/x/java/"), 1);
    }
};

QTEST_GUILESS_MAIN(JavaChoiceTest)

#include "JavaChoice_test.moc"
