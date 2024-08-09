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
        this->setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
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
    // qDebug() << "UID is "<< UID;

    // get list of usb storage devices

    // first get list of mounted devices
    QStringList partitionlist;
    QStringList gvfslist;
    QString file_content;

    file_content = runCmd("df --local --output=source,target,size -H | grep /dev/").str;
    partitionlist = file_content.split("\n");
    // qDebug() << "Partition list: " << partitionlist;
    // now build list for gvfs devices mtp and gphoto
    file_content = runCmd("ls -1 --color=never /run/user/" + UID + "/gvfs |grep mtp").str;
    file_content.append(runCmd("ls -1 --color=never /run/user/" + UID + "/gvfs |grep gphoto").str);
    gvfslist = file_content.split("\n");
    // qDebug() << gvfslist;
    for (const QString &item : gvfslist) {
        if (!item.isEmpty()) {
            //            qDebug() << "Dev" << dev;
            QString devpath = QString("/run/user/" + UID + "/" + item);
            //            qDebug() << "DevPath" << devpath;
            QString addtopartitionlist = QString(devpath + " " + item + " ");
            partitionlist << addtopartitionlist;
        }
    }

    // qDebug() << "Partition list: " << partitionlist;

    // now check usb status of each item.  if usb is yes, add to usb list, if optical is yes, add to optical list

    bool isUSB {};
    bool isCD {};
    bool isMMC {};
    bool isGPHOTO {};
    bool isMTP {};
    QString devicename;
    QListWidgetItem *list_item = nullptr;
    QString point;
    QString size;
    QString label;
    QString partition;
    QString model;

    for (const QString &item : partitionlist) {
        if (item.startsWith("/dev/mapper/rootfs") || item.startsWith("tmpfs") || item.startsWith("df: ")) {
            continue;
        }
        // devicename = item.simplified().section(' ', 0 ,0).section('/', 2, 2);  //gives us device designation (sda,
        // sdb, etc..)
        point = item.simplified().section(' ', 1, 1);
        size = item.simplified().section(' ', 2, 2);
        partition = item.simplified().section(' ', 0, 0);
        label = runCmd("udevadm info --query=property " + partition + " |grep ID_FS_LABEL=").str.section('=', 1, 1);
        // qDebug() << "Mountpoint: " << point;
        // qDebug() << "Size: " << size;
        // qDebug() << "Partition: " << partition;
        // qDebug() << "Label: " << label;
        // isUSB = system("udevadm info --query=property --path=/sys/block/" + devicename.toUtf8() + " | grep -qE
        // '^DEVPATH=.*/usb[0-9]+/'") == 0;
        isUSB = system("udevadm info --query=property " + partition.toUtf8() + " | grep -qE '^DEVPATH=.*/usb[0-9]+/'")
                == 0;
        // isCD = system("udevadm info --query=property --path=/sys/block/" + devicename.toUtf8() + " | grep -q
        // ID_TYPE=cd") == 0;
        isCD = system("udevadm info --query=property " + partition.toUtf8() + " | grep -q ID_TYPE=cd") == 0;
        isMMC = system("udevadm info --query=property " + partition.toUtf8() + " | grep -q ID_DRIVE_FLASH_SD=") == 0;
        isGPHOTO = point.section(':', 0, 0) == "gphoto2";
        isMTP = point.section(':', 0, 0) == "mtp";
        // model = runCmd("udevadm info --query=property --path=/sys/block/" + devicename.toUtf8() + " | grep
        // ID_MODEL=").str.section('=',1,1);

        model = runCmd("udevadm info --query=property " + partition.toUtf8() + " | grep ID_MODEL=")
                    .str.section('=', 1, 1);
        QString DEVTYPE = runCmd("udevadm info --query=property " + partition.toUtf8() + " |grep DEVTYPE=")
                              .str.section("/", -1, -1);
        // correction for partitionless disks, some devices like ereaders and some usb sticks don't have partitions
        if (DEVTYPE == "DEVTYPE=disk") {
            devicename = runCmd("udevadm info --query=property " + partition.toUtf8() + " |grep DEVPATH=")
                             .str.section("/", -1, -1);
        } else {
            devicename = runCmd("udevadm info --query=property " + partition.toUtf8() + " |grep DEVPATH=")
                             .str.section("/", -2, -2);
        }

        if (isGPHOTO || isMTP) {
            model = point.section("=", 1, 1);
            devicename = model;
            isUSB = true;
            label = "";
        }

        // qDebug() << "Device name: " << devicename;
        // qDebug() << "Model: " << model;
        // qDebug() << "Is USB: " << isUSB;
        // qDebug() << "Is CD: " << isCD;
        // qDebug() << "Is MMC: " << isMMC;
        // qDebug() << "Is MTP: " << isMTP;
        // qDebug() << "Is GPHOTO: " << isGPHOTO;

        if (isUSB || isCD || isMMC) {
            list_item = new QListWidgetItem(ui->mountlistview);
            QString item2 = QString(model + " " + size + " " + tr("Volume").toUtf8() + " " + label);
            list_item->setText(item2);
            //            qDebug() << "widget item: " << item2;
            QString data;
            if (isUSB) {
                if (isMTP) {
                    list_item->setIcon(QIcon::fromTheme("multimedia-player"));
                    data = QString("mtp;" + partition + ";" + point + ";" + model);
                    list_item->setData(Qt::UserRole, data);
                    list_item->setToolTip(point);
                } else if (isGPHOTO) {
                    list_item->setIcon(QIcon::fromTheme("camera-photo"));
                    data = QString("gphoto2;" + partition + ";" + point + ";" + model);
                    list_item->setData(Qt::UserRole, data);
                    list_item->setToolTip(point);
                } else {
                    list_item->setIcon(QIcon::fromTheme("drive-removable-media"));
                    data = QString("usb;" + partition + ";" + devicename + ";" + model);
                    list_item->setData(Qt::UserRole, data);
                    list_item->setToolTip(point);
                }
            }
            if (isCD) {
                list_item->setIcon(QIcon::fromTheme("media-optical"));
                data = QString("cd;" + partition + ";" + devicename + ";" + model);
                list_item->setData(Qt::UserRole, data);
                list_item->setToolTip(point);
            }
            if (isMMC) {
                list_item->setIcon(QIcon::fromTheme("media-flash"));
                data = QString("mmc;" + partition + ";" + devicename + ";" + model);
                list_item->setData(Qt::UserRole, data);
                list_item->setToolTip(point);
            }
            //            qDebug() << "Data list: " << data;
        }
    }

    if (ui->mountlistview->count() > 0) {
        ui->mountlistview->item(0)->setSelected(true);
    } else {
        list_item = new QListWidgetItem(ui->mountlistview);
        list_item->setText(tr("No Removable Device"));
        list_item->setIcon(QIcon::fromTheme("process-stop"));
        list_item->setData(Qt::UserRole, QString());
    }
    this->show();
    this->raise();
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
        this->hide();
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
        out = runCmd("umount " + partitiondevice).exit_code;
    }
    if (type == "usb") {
        out = runCmd("umount /dev/" + mountdevice + "?*").exit_code;
        //        qDebug() << "unmount paritions exit code" << out;
        if (out != 0) {
            // try just unmounting the device
            out = runCmd("umount /dev/" + mountdevice).exit_code;
            //            qDebug() << "unmount device exit code" << out;
            if (out != 0) {
                QString out2 = runCmd("cat /etc/mtab | grep -q " + mountdevice + " && echo $?").str;
                //                qDebug() << "out2 is " << out2;
                if (out2.isEmpty()) {
                    out = 0;
                }
            }
        }
    }

    if (type == "cd") {
        out = runCmd("eject " + partitiondevice).exit_code;
    }

    // if type is gphoto or mtp, use gvfs-mount -u to unmount
    if (type == "mtp" || type == "gphoto2") {
        out = runCmd("gio mount -u /run/user/$UID/gvfs/" + mountdevice).exit_code;
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
    this->hide();
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
    if (pos.y() + this->size().height() > screen->availableVirtualGeometry().height()) {
        pos.setY(screen->availableVirtualGeometry().height() - this->size().height());
    }
    if (pos.x() + this->size().width() > screen->availableVirtualGeometry().width()) {
        pos.setX(screen->availableVirtualGeometry().width() - this->size().width());
    }
    this->move(pos);
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
        if (!this->isActiveWindow()) {
            this->hide();
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
        this->hide();
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
        QStringLiteral("/usr/share/doc/mx-usb-unmounter/license.html"), tr("%1 License").arg(this->windowTitle()));
}
