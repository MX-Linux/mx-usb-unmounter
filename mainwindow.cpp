#include <QDebug>
#include <QDir>
#include <QKeyEvent>
#include <QMessageBox>
#include <QScreen>

#include "about.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(const QString &arg1, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::MainWindow)
{
    QApplication::setQuitOnLastWindowClosed(false);
    if (arg1 == "--help" || arg1 == "-h") {
        about();
        QCoreApplication::quit();
    } else {
        ui->setupUi(this);
        createActions();
        createMenu();
        connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);
        connect(ui->mountlistview, &QListWidget::itemActivated, this, &MainWindow::mountlistviewItemActivated);
        connect(ui->cancel, &QPushButton::clicked, this, &MainWindow::cancelPressed);
        trayIcon->show();
        setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    }
}

// util function for getting bash command output and error code
Output MainWindow::runCmd(const QString &cmd)
{
    if (proc.state() != QProcess::NotRunning) {
        qDebug() << "Process already running:" << proc.program() << proc.arguments();
        return {};
    }
    QEventLoop loop;
    connect(&proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), &loop, &QEventLoop::quit);
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start("/bin/bash", {"-c", cmd});
    loop.exec();
    return {proc.exitCode(), proc.readAll().trimmed()};
}

void MainWindow::start()
{
    if (proc.state() != QProcess::NotRunning) {
        return;
    }

    ui->mountlistview->clear();
    UID = runCmd("echo $UID").str;
    // qDebug() << "UID is" << UID;

    // Get list of mounted devices
    QStringList partitionList
        = runCmd("df --local --output=source,target,size -H | grep /dev/").str.split('\n', Qt::SkipEmptyParts);
    QStringList gvfslist = runCmd(QString("ls -1 --color=never /run/user/%1/gvfs | grep -E 'mtp|gphoto'").arg(UID))
                               .str.split('\n', Qt::SkipEmptyParts);

    // Append gvfs devices to partition list
    for (const QString &item : gvfslist) {
        if (!item.isEmpty()) {
            partitionList << QString("/run/user/%1/gvfs/%2 %2").arg(UID, item);
        }
    }

    // Process device properties and populate list
    for (const QString &item : partitionList) {
        if (item.startsWith("/dev/mapper/rootfs") || item.startsWith("tmpfs") || item.startsWith("df: ")) {
            continue;
        }

        QStringList itemParts = item.simplified().split(' ', Qt::SkipEmptyParts);
        if (itemParts.size() < 3) {
            continue; // Skip incomplete items
        }

        QString partition = itemParts.at(0);
        QString point = itemParts.at(1);
        QString size = itemParts.at(2);

        const QString udevInfo = runCmd("udevadm info --query=property " + partition).str;
        QString label = udevInfo.section("ID_FS_LABEL=", 1, 1).section('\n', 0, 0);
        QString model = udevInfo.section("ID_MODEL=", 1, 1).section('\n', 0, 0);
        QString devType = udevInfo.section("DEVTYPE=", 1, 1).section('\n', 0, 0);
        // Correction for partitionless disks, some devices like ereaders and some usb sticks don't have partitions
        QString deviceName = (devType == "disk")
                                 ? udevInfo.section("DEVPATH=", 1, 1).section('\n', 0, 0).section('/', -1)
                                 : udevInfo.section("DEVPATH=", 1, 1).section('\n', 0, 0).section('/', -2, -2);
        bool isUSB
            = udevInfo.contains(QRegularExpression("^DEVPATH=.*/usb[0-9]+/", QRegularExpression::MultilineOption));
        bool isCD = udevInfo.contains("ID_TYPE=cd");
        bool isMMC = udevInfo.contains("ID_DRIVE_FLASH_SD=");
        bool isGPHOTO = point.startsWith("gphoto2:");
        bool isMTP = point.startsWith("mtp:");

        // Adjust for GPHOTO and MTP devices
        if (isGPHOTO || isMTP) {
            model = point.section('=', 1);
            deviceName = model;
            isUSB = true;
            label.clear();
        }

        // qDebug() << "Device name:" << deviceName;
        // qDebug() << "Model:" << model;
        // qDebug() << "Mount point:" << point;
        // qDebug() << "Is USB:" << isUSB;
        // qDebug() << "Is CD:" << isCD;
        // qDebug() << "Is MMC:" << isMMC;
        // qDebug() << "Is MTP:" << isMTP;
        // qDebug() << "Is GPHOTO:" << isGPHOTO;
        if (isUSB || isCD || isMMC) {
            auto *list_item = new QListWidgetItem(ui->mountlistview);
            const QString itemText = QString("%1 %2 %3 %4").arg(model, size, tr("Volume"), label);
            list_item->setText(itemText);

            QString data;
            QString iconName;
            if (isUSB) {
                if (isMTP) {
                    iconName = "multimedia-player";
                    data = QString("mtp;%1;%2;%3").arg(partition, point, model);
                } else if (isGPHOTO) {
                    iconName = "camera-photo";
                    data = QString("gphoto2;%1;%2;%3").arg(partition, point, model);
                } else {
                    iconName = "drive-removable-media";
                    data = QString("usb;%1;%2;%3").arg(partition, deviceName, model);
                }
            } else if (isCD) {
                iconName = "media-optical";
                data = QString("cd;%1;%2;%3").arg(partition, deviceName, model);
            } else if (isMMC) {
                iconName = "media-flash";
                data = QString("mmc;%1;%2;%3").arg(partition, deviceName, model);
            }

            list_item->setIcon(QIcon::fromTheme(iconName));
            list_item->setData(Qt::UserRole, data);
            list_item->setToolTip(point);
        }
    }

    // Update UI with the device list
    if (ui->mountlistview->count() > 0) {
        ui->mountlistview->item(0)->setSelected(true);
    } else {
        auto *list_item = new QListWidgetItem(ui->mountlistview);
        list_item->setText(tr("No Removable Device"));
        list_item->setIcon(QIcon::fromTheme("process-stop"));
        list_item->setData(Qt::UserRole, QString());
    }

    show();
    raise();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::mountlistviewItemActivated(QListWidgetItem *item)
{
    // if device is mmc, just unmount. if usb, also poweroff. if device is cd/dvd, eject as well, then remove list item
    // if no device, then exit
    QString cmd2;
    QString cmd3;
    QString cmd4;
    QString point = QString(item->text());
    // qDebug() << "clicked mount point" << point;
    cmd2 = tr("Unmounting " + point.toUtf8());
    cmd3 = tr(point.toUtf8() + " is Safe to Remove");
    cmd4 = tr("Other partitions still mounted on device");
    QString title = tr("MX USB Unmounter");
    QString type = item->data(Qt::UserRole).toString().section(";", 0, 0);
    // qDebug() << item->data(Qt::UserRole).toString();
    // qDebug() << "type is" << type;
    QString pattern("compact_flash|CF|sd|sdhc|MMC|ms|sdxc|xD");
    QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);

    bool poweroff = true;

    if (item->data(Qt::UserRole).toString().isEmpty()) {
        hide();
        return;
    }
    // run operation on selected device

    QString mountdevice = item->data(Qt::UserRole).toString().section(";", 2, 2);
    QString partitiondevice = item->data(Qt::UserRole).toString().section(";", 1, 1);
    QString model = item->data(Qt::UserRole).toString().section(";", 3, 3);
    // qDebug() << "Mount device is" << mountdevice;
    // qDebug() << "Partion device is" << partitiondevice;
    // qDebug() << type;
    int out = 0;

    QString powertest = runCmd("udevadm info --query=property /dev/" + mountdevice + " |grep DEVLINKS").str;
    QRegularExpressionMatch match = re.match(powertest, 0);
    if (match.hasMatch()) {
        poweroff = false;
    }

    // qDebug() << "power off is " << poweroff;

    // if item is mmc, unmount only, don't "eject"
    if (type == "mmc") {
        out = runCmd("umount " + partitiondevice).exitCode;
    }
    if (type == "usb") {
        out = runCmd("umount /dev/" + mountdevice + "?*").exitCode;
        // qDebug() << "unmount paritions exit code" << out;
        if (out != 0) {
            // try just unmounting the device
            out = runCmd("umount /dev/" + mountdevice).exitCode;
            // qDebug() << "unmount device exit code" << out;
            if (out != 0) {
                QString out2 = runCmd("cat /etc/mtab | grep -q " + mountdevice + " && echo $?").str;
                // qDebug() << "out2 is " << out2;
                if (out2.isEmpty()) {
                    out = 0;
                }
            }
        }
    }

    if (type == "cd") {
        out = runCmd("eject " + partitiondevice).exitCode;
    }

    // if type is gphoto or mtp, use gvfs-mount -u to unmount
    if (type == "mtp" || type == "gphoto2") {
        out = runCmd("gio mount -u /run/user/$UID/gvfs/" + mountdevice).exitCode;
    }

    // qDebug() << "out is " << out;

    if (out == 0) {
        // if device is usb, go ahead and power off "eject" and notify user
        if (type == "usb") {
            if (poweroff) {
                system("udisksctl power-off -b /dev/" + mountdevice.toUtf8());
            }
            system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd2.toUtf8() + "'");
            system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd3.toUtf8() + "'");
        }
        // if device is mtp or gphoto2, say unmount was a success
        if (type == "mtp" || type == "gphoto2") {
            system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd2.toUtf8() + "'");
            system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd3.toUtf8() + "'");
        }
        // if device is CD, done, and notify user of success
        if (type == "cd") {
            system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd2.toUtf8() + "'");
            system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd3.toUtf8() + "'");
        }

        // if device is mmc, check for additional mounted partitions.  If none, say safe to eject.  If more found, say
        // so.
        if (type == "mmc") {
            QString mmc_check;
            mmc_check = runCmd("df --local --output=source,target,size -H 2>/dev/null| grep " + mountdevice).str;
            if (mmc_check.isEmpty()) {
                system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd2.toUtf8() + "'");
                system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd3.toUtf8() + "'");
            } else {
                system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd2.toUtf8() + "'");
                system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd4.toUtf8() + " "
                       + model.toUtf8() + "'");
            }
        }

    } else { // if umount operation failed, say so
        // qDebug() << "Warning";
        cmd3 = tr("Unable to  Unmount, Device in Use");
        system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd3.toUtf8() + "'");
    }
    hide();
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    setPosition();
    switch (reason) {
    case QSystemTrayIcon::DoubleClick:
    case QSystemTrayIcon::MiddleClick:
    case QSystemTrayIcon::Trigger:
        start();
        break;
    default:
        break;
    }
}

void MainWindow::createActions()
{
    aboutAction = new QAction(QIcon::fromTheme("help-about"), tr("About"), this);
    helpAction = new QAction(QIcon::fromTheme("help-browser"), tr("Help"), this);
    listDevicesAction = new QAction(QIcon::fromTheme("drive-removable-media"), tr("List Devices"), this);
    quitAction = new QAction(QIcon::fromTheme("gtk-quit"), tr("Quit"), this);
    toggleAutostartAction = new QAction(QIcon::fromTheme("preferences-system"), tr("Enable Autostart?"), this);

    connect(aboutAction, &QAction::triggered, this, &MainWindow::about);
    connect(helpAction, &QAction::triggered, this, &MainWindow::help);
    connect(listDevicesAction, &QAction::triggered, this, &MainWindow::start);
    connect(quitAction, &QAction::triggered, QApplication::instance(), &QGuiApplication::quit);
    connect(toggleAutostartAction, &QAction::triggered, this, &MainWindow::toggleAutostart);
}

void MainWindow::createMenu()
{
    menu = new QMenu(this);
    menu->addAction(listDevicesAction);
    menu->addAction(toggleAutostartAction);
    menu->addAction(helpAction);
    menu->addAction(aboutAction);
    menu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon("/usr/share/pixmaps/usb-unmounter.svg"));
    trayIcon->setContextMenu(menu);
    trayIcon->setToolTip(tr("Unmount"));
}

void MainWindow::help()
{
    QString url = "file:///usr/share/doc/mx-usb-unmounter/mx-usb-unmounter.html";
    QLocale locale;
    QString lang = locale.bcp47Name();
    if (lang.startsWith("fr")) {
        url = "https://mxlinux.org/wiki/help-files/help-mx-d%C3%A9monte-usb";
    }
    displayDoc(url, tr("%1 Help").arg(tr("MX USB Unmounter")));
}

void MainWindow::setPosition()
{
    QPoint pos = QCursor::pos();
    QScreen *screen = QGuiApplication::screenAt(pos);
    if (pos.y() + size().height() > screen->availableVirtualGeometry().height()) {
        pos.setY(screen->availableVirtualGeometry().height() - size().height());
    }
    if (pos.x() + size().width() > screen->availableVirtualGeometry().width()) {
        pos.setX(screen->availableVirtualGeometry().width() - size().width());
    }
    move(pos);
}

void MainWindow::toggleAutostart()
{
    QString local_file = QDir::homePath() + "/.config/autostart/mx-usb-unmounter.desktop";
    if (QMessageBox::Yes == QMessageBox::question(nullptr, tr("Autostart Settings"), tr("Enable Autostart?"))) {
        QFile::remove(local_file);
    } else {
        QFile::copy("/usr/share/mx-usb-unmounter/mx-usb-unmounter.desktop", local_file);
    }
}

// implement change event that closes app when window loses focus
void MainWindow::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            hide();
        }
    }
}

void MainWindow::cancelPressed()
{
    hide();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        hide();
    }
}

void MainWindow::about()
{
    QString version = runCmd("dpkg-query --show mx-usb-unmounter").str.simplified().section(' ', 1, 1);
    displayAboutMsgBox(
        tr("About MX USB Unmounter"),
        "<p align=\"center\"><b><h2>" + tr("MX USB Unmounter") + "</h2></b></p><p align=\"center\">" + tr("Version: ")
            + version + "</p><p align=\"center\"><h3>" + tr("Quickly Unmount Removable Media")
            + "</h3></p><p align=\"center\"><a href=\"http://mxlinux.org\">http://mxlinux.org</a><br /></p>"
              "<p align=\"center\">"
            + tr("Copyright (c) MX Linux") + "<br /><br /></p>",
        QStringLiteral("/usr/share/doc/mx-usb-unmounter/license.html"), tr("%1 License").arg(windowTitle()));
}
