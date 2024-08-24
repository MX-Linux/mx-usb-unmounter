#include <QDBusArgument>
#include <QDebug>
#include <QDir>
#include <QKeyEvent>
#include <QMessageBox>
#include <QScreen>

#include "about.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "version.h"

MainWindow::MainWindow(const QString &arg1, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::MainWindow)
{
    QApplication::setQuitOnLastWindowClosed(false);

    // Handle command line arguments for help
    if (arg1 == "--help" || arg1 == "-h") {
        about();
        exit(0);
    }

    // Setup UI and window properties
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);

    // Create actions and menu
    createActions();
    createMenu();

    // Connect signals to slots
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);
    connect(ui->mountlistview, &QListWidget::itemActivated, this, &MainWindow::mountlistviewItemActivated);
    connect(ui->cancel, &QPushButton::clicked, this, &MainWindow::cancelPressed);

    // Show or hide tray icon based on settings and device list
    bool isHidden = settings.value("Hide", false).toBool();
    if (isHidden) {
        deviceMonitor();
        trayIcon->setVisible(hasDevices());
    } else {
        trayIcon->show();
    }
}

// Util function for getting bash command output and error code
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
    listDevices();
    show();
    raise();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::mountlistviewItemActivated(QListWidgetItem *item)
{
    const QString itemData = item->data(Qt::UserRole).toString();
    if (itemData.isEmpty()) {
        hide();
        return;
    }

    const QStringList itemDataParts = itemData.split(';');
    if (itemDataParts.size() < 4) {
        qWarning() << "Invalid item data format:" << itemData;
        return;
    }

    const QString type = itemDataParts.at(0);
    const QString partitionDevice = itemDataParts.at(1);
    const QString mountDevice = itemDataParts.at(2);
    const QString model = itemDataParts.at(3);
    const QString point = item->text();
    const QString title = tr("MX USB Unmounter");

    const QString unmountingMsg = tr("Unmounting %1").arg(point);
    const QString safeToRemoveMsg = tr("%1 is Safe to Remove").arg(point);
    const QString otherPartitionsMountedMsg = tr("Other partitions still mounted on device");

    const QRegularExpression re("compact_flash|CF|sd|sdhc|MMC|ms|sdxc|xD", QRegularExpression::CaseInsensitiveOption);
    const QString powerTestCmd = QString("udevadm info --query=property /dev/%1 | grep DEVLINKS").arg(mountDevice);
    const bool powerOff = !re.match(runCmd(powerTestCmd).str).hasMatch();

    int exitCode = 0;
    auto unmountDevice = [&](const QString &device) { exitCode = runCmd("umount " + device).exitCode; };
    const QStringList notifyArgs = {"-i", "drive-removable-media", title};
    QProcess::execute("notify-send", notifyArgs + QStringList() << unmountingMsg);

    if (type == "mmc") {
        unmountDevice(partitionDevice);
    } else if (type == "usb") {
        unmountDevice("/dev/" + mountDevice + "?*");
        if (exitCode != 0) {
            unmountDevice("/dev/" + mountDevice);
            if (exitCode != 0 && QProcess::execute("grep", {"-q", mountDevice, "/etc/mtab"}) != 0) {
                exitCode = 0; // Reset exitCode if device is not in mtab
            }
        }
    } else if (type == "cd") {
        exitCode = QProcess::execute("eject", {partitionDevice});
    } else if (type == "mtp" || type == "gphoto2") {
        exitCode = QProcess::execute("gio", {"mount", "-u", "/run/user/$UID/gvfs/" + mountDevice});
    }

    // qDebug() << "Exit code is " << exitCode;
    if (exitCode == 0) {
        if (type == "usb" && powerOff) {
            QProcess::execute("udisksctl", {"power-off", "-b", "/dev/" + mountDevice});
        }
        QString notificationMessage = safeToRemoveMsg;
        if (type == "mmc") {
            const QString mmcCheckOutput
                = runCmd(QString("df --local --output=source,target,size -H 2>/dev/null | grep -E '^/dev/%1'")
                             .arg(mountDevice))
                      .str;
            const bool hasOtherPartitions = !mmcCheckOutput.isEmpty();
            notificationMessage
                = hasOtherPartitions ? QString("%1 %2").arg(otherPartitionsMountedMsg, model) : safeToRemoveMsg;
        }
        QProcess::execute("notify-send", notifyArgs + QStringList() << notificationMessage);
    } else {
        const QString errorMsg = tr("Unable to unmount, device in use");
        QProcess::execute("notify-send", {"-i", "drive-removable-media", title, errorMsg});
    }
    hide();
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    setPosition();
    if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::MiddleClick
        || reason == QSystemTrayIcon::Trigger) {
        start();
    }
}

void MainWindow::createActions()
{
    aboutAction = new QAction(QIcon::fromTheme("help-about"), tr("About"), this);
    helpAction = new QAction(QIcon::fromTheme("help-browser"), tr("Help"), this);
    listDevicesAction = new QAction(QIcon::fromTheme("drive-removable-media"), tr("List Devices"), this);
    quitAction = new QAction(QIcon::fromTheme("gtk-quit"), tr("Quit"), this);

    const QString autostartFile = QDir::homePath() + "/.config/autostart/mx-usb-unmounter.desktop";
    toggleAutostartAction = new QAction(QIcon::fromTheme("preferences-system"),
        tr(QFile::exists(autostartFile) ? "Disable Autostart" : "Enable Autostart"), this);

    toggleHideAction = new QAction(QIcon::fromTheme("keyboard-hide-symbolic"),
        settings.value("Hide", false).toBool() ? tr("Always show icon") : tr("Hide icon when not in use"), this);

    connect(aboutAction, &QAction::triggered, this, &MainWindow::about);
    connect(helpAction, &QAction::triggered, this, &MainWindow::help);
    connect(listDevicesAction, &QAction::triggered, this, &MainWindow::start);
    connect(quitAction, &QAction::triggered, QApplication::instance(), &QGuiApplication::quit);
    connect(toggleAutostartAction, &QAction::triggered, this, &MainWindow::toggleAutostart);
    connect(toggleHideAction, &QAction::triggered, this, &MainWindow::toggleHideIcon);
}

void MainWindow::createMenu()
{
    menu = new QMenu(this);
    menu->addAction(listDevicesAction);
    menu->addAction(toggleAutostartAction);
    menu->addAction(toggleHideAction);
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
    const QString autostartFile = QDir::homePath() + "/.config/autostart/mx-usb-unmounter.desktop";
    const bool autostartEnabled = QFile::exists(autostartFile);

    QString message;
    QString title;
    bool success = false;

    if (autostartEnabled) {
        success = QFile::remove(autostartFile);
        message = success ? tr("Autostart has been disabled.") : tr("Failed to disable autostart.");
        title = tr("Autostart Disabled");
        toggleAutostartAction->setText(tr("Enable Autostart"));
    } else {
        success = QFile::copy("/usr/share/mx-usb-unmounter/mx-usb-unmounter.desktop", autostartFile);
        message = success ? tr("Autostart has been enabled.") : tr("Failed to enable autostart.");
        title = tr("Autostart Enabled");
        toggleAutostartAction->setText(tr("Disable Autostart"));
    }

    QMessageBox::information(this, title, message);
}

void MainWindow::toggleHideIcon()
{
    QString message;
    if (toggleHideAction->text() == tr("Always show icon")) {
        message = tr("Application icon will stay visible all the time.");
        settings.setValue("Hide", false);
        toggleHideAction->setText(tr("Hide icon when not in use"));
        disconnectFromDBus();
    } else {
        toggleHideAction->setText(tr("Always show icon"));
        settings.setValue("Hide", true);
        if (!hasDevices()) {
            message = tr("Application icon is now hidden and will reappear when a new device is connected.");
            trayIcon->setVisible(false);
        } else {
            message = tr("Application icon will be hidden when there are no devices plugged in.");
        }
        deviceMonitor();
    }

    QMessageBox::information(this, tr("Icon Visibility"), message);
}

bool MainWindow::hasDevices()
{
    listDevices();
    return ui->mountlistview->count() > 0
           && (ui->mountlistview->count() != 1 || ui->mountlistview->item(0)->text() != tr("No Removable Device"));
}

void MainWindow::disconnectFromDBus()
{
    if (!systemBus) {
        return;
    }
    systemBus->disconnect(serviceName, objectPath, interfaceName, "InterfacesRemoved", this,
                          SLOT(onInterfacesRemoved(QDBusMessage)));
}

void MainWindow::deviceMonitor()
{
    systemBus = new QDBusConnection(QDBusConnection::systemBus());

    if (!systemBus->isConnected()) {
        qWarning() << "Cannot connect to the D-Bus system bus";
        return;
    }
    if (!systemBus->connect(serviceName, objectPath, interfaceName, "InterfacesRemoved", this,
                            SLOT(onInterfacesRemoved(QDBusMessage)))) {
        qWarning() << "Failed to connect to the 'InterfacesRemoved' signal";
    }
}

void MainWindow::onInterfacesRemoved(const QDBusMessage &message)
{
    if (message.arguments().size() < 2) {
        qWarning() << "Invalid message format from D-Bus";
        return;
    }
    QString arg = message.arguments().at(1).toString();
    if (!arg.isEmpty()) {
        trayIcon->setVisible(hasDevices());
    }
}

// Implement change event that closes app when window loses focus
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
    QString aboutMessage
        = QString("<p align=\"center\"><b><h2>%1</h2></b></p>"
                  "<p align=\"center\">%2: %3</p>"
                  "<p align=\"center\"><h3>%4</h3></p>"
                  "<p align=\"center\"><a href=\"%5\">%5</a><br /></p>"
                  "<p align=\"center\">%6<br /><br /></p>")
              .arg(tr("MX USB Unmounter"), tr("Version"), VERSION, tr("Quickly Unmount Removable Media"),
                   "http://mxlinux.org", tr("Copyright (c) MX Linux"));

    displayAboutMsgBox(tr("About MX USB Unmounter"), aboutMessage,
                       QStringLiteral("/usr/share/doc/mx-usb-unmounter/license.html"),
                       tr("%1 License").arg(windowTitle()));
}

void MainWindow::listDevices()
{
    if (proc.state() != QProcess::NotRunning) {
        return;
    }

    ui->mountlistview->clear();
    UID = runCmd("echo $UID").str;
    // qDebug() << "UID is" << UID;

    // Get list of mounted devices
    QStringList partitionList = runCmd("df --local --output=source,target,size -H 2>/dev/null | grep /dev/")
                                    .str.split('\n', Qt::SkipEmptyParts);
    QStringList gvfslist
        = runCmd(QString("ls -1 --color=never /run/user/%1/gvfs 2>/dev/null | grep -E 'mtp|gphoto'").arg(UID))
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
}
