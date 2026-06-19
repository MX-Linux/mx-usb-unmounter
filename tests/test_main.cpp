#include <QTest>
#include <QString>

#include "../deviceutils.h"
#include "../mainwindow.h"

class TestMxUsbUnmounter : public QObject
{
    Q_OBJECT

private slots:
    void testOutputStruct();
    void testOutputDefault();
    void testParseDeviceDataValid();
    void testParseDeviceDataInvalid();
    void testIdPathLine();
    void testShouldPowerOff();
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

void TestMxUsbUnmounter::testParseDeviceDataValid()
{
    const devutils::DeviceData d
        = devutils::parseDeviceData(QStringLiteral("usb;/dev/sdb1;sdb;Kingston DataTraveler"));
    QVERIFY(d.valid);
    QCOMPARE(d.type, QStringLiteral("usb"));
    QCOMPARE(d.partitionDevice, QStringLiteral("/dev/sdb1"));
    QCOMPARE(d.mountDevice, QStringLiteral("sdb"));
    QCOMPARE(d.model, QStringLiteral("Kingston DataTraveler"));
}

void TestMxUsbUnmounter::testParseDeviceDataInvalid()
{
    QVERIFY(!devutils::parseDeviceData(QString()).valid);                            // empty
    QVERIFY(!devutils::parseDeviceData(QStringLiteral("garbage")).valid);            // no separators
    QVERIFY(!devutils::parseDeviceData(QStringLiteral("usb;/dev/sdb1;sdb")).valid);  // only 3 fields
}

void TestMxUsbUnmounter::testIdPathLine()
{
    const QString udev = QStringLiteral("DEVNAME=/dev/sdb\n"
                                        "ID_PATH=pci-0000:00:14.0-usb-0:2:1.0-scsi-0:0:0:0\n"
                                        "ID_MODEL=DataTraveler\n");
    QCOMPARE(devutils::idPathLine(udev),
             QStringLiteral("ID_PATH=pci-0000:00:14.0-usb-0:2:1.0-scsi-0:0:0:0"));

    // No ID_PATH present -> empty
    QVERIFY(devutils::idPathLine(QStringLiteral("DEVNAME=/dev/sdb\nID_MODEL=x\n")).isEmpty());
    QVERIFY(devutils::idPathLine(QString()).isEmpty());
}

void TestMxUsbUnmounter::testShouldPowerOff()
{
    // A regular USB stick/HDD (no card-reader marker in ID_PATH) is powered off.
    QVERIFY(devutils::shouldPowerOff(
        QStringLiteral("ID_PATH=pci-0000:00:14.0-usb-0:2:1.0-scsi-0:0:0:0")));

    // A card reader (ID_PATH contains an mmc marker) is NOT powered off.
    QVERIFY(!devutils::shouldPowerOff(QStringLiteral("ID_PATH=pci-0000:00:1d.0-mmc-MMC")));

    // No ID_PATH found -> default to powering off.
    QVERIFY(devutils::shouldPowerOff(QString()));
}

QTEST_MAIN(TestMxUsbUnmounter)
#include "test_main.moc"
