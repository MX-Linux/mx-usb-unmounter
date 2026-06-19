#include <QTest>
#include <QString>

// Include the project header to verify compilation
#include "../mainwindow.h"

class TestMxUsbUnmounter : public QObject
{
    Q_OBJECT

private slots:
    void testOutputStruct();
    void testOutputDefault();
};

void TestMxUsbUnmounter::testOutputStruct()
{
    // Verify Output struct can hold exit code and string
    const Output result1{0, QStringLiteral("success")};
    QCOMPARE(result1.exitCode, 0);
    QCOMPARE(result1.str, QStringLiteral("success"));

    const Output result2{-1, QString()};
    QCOMPARE(result2.exitCode, -1);
    QVERIFY(result2.str.isEmpty());
}

void TestMxUsbUnmounter::testOutputDefault()
{
    // Verify default-constructed Output
    const Output result{};
    QCOMPARE(result.exitCode, 0);
    QVERIFY(result.str.isEmpty());
}

QTEST_MAIN(TestMxUsbUnmounter)
#include "test_main.moc"
