#pragma once

// Pure, side-effect-free helpers extracted from MainWindow so they can be unit
// tested without spawning subprocesses or constructing the UI.

#include <QRegularExpression>
#include <QString>
#include <QStringList>

namespace devutils
{

// Payload stored in each list item's Qt::UserRole, built in listDevices() as
// "type;partitionDevice;mountDevice;model".
struct DeviceData {
    bool valid = false;
    QString type;
    QString partitionDevice;
    QString mountDevice;
    QString model;
};

// Parse the ';'-separated item payload. Returns valid == false for empty input
// or fewer than the four expected fields.
inline DeviceData parseDeviceData(const QString &itemData)
{
    DeviceData data;
    const QStringList parts = itemData.split(';');
    if (parts.size() < 4) {
        return data;
    }
    data.valid = true;
    data.type = parts.at(0);
    data.partitionDevice = parts.at(1);
    data.mountDevice = parts.at(2);
    data.model = parts.at(3);
    return data;
}

// Extract the "ID_PATH=..." line from `udevadm info --query=property` output,
// or an empty string if there is none.
inline QString idPathLine(const QString &udevOutput)
{
    return udevOutput.split('\n', Qt::SkipEmptyParts)
        .filter(QRegularExpression(QStringLiteral("^ID_PATH=")))
        .value(0);
}

// A drive is powered off after unmount unless its ID_PATH marks it as a
// card-reader-style device (SD/MMC/CF/MS/xD), which has nothing to spin down.
inline bool shouldPowerOff(const QString &idPath)
{
    static const QRegularExpression cardReader(
        QStringLiteral("compact_flash|CF|(?<!s)sd|sdhc|MMC|ms|sdxc|xD"),
        QRegularExpression::CaseInsensitiveOption);
    return !cardReader.match(idPath).hasMatch();
}

} // namespace devutils
