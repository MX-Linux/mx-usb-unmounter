#include "usbunmounter.h"
#include "ui_usbunmounter.h"
#include <QDebug>
#include <QKeyEvent>
#include <QMessageBox>

usbunmounter::usbunmounter(QString arg1, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::usbunmounter)
{
    if ( arg1 == "--help" or arg1 == "-h" ) {
        if (about() == 1) {
            exit(0);
        }
    } else {
        ui->setupUi(this);
        this->setWindowIcon(QIcon::fromTheme("mx-usb-unmounter"));
        this->setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
        this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
        this->move(QCursor::pos());
        is_start = true;
        start();
    }
}

// util function for getting bash command output and error code
Output usbunmounter::runCmd(QString cmd)
{
    QProcess *proc = new QProcess();
    QEventLoop loop;
    connect(proc, SIGNAL(finished(int)), &loop, SLOT(quit()));
    proc->setReadChannelMode(QProcess::MergedChannels);
    proc->start("/bin/bash", QStringList() << "-c" << cmd);
    loop.exec();
    Output out = {proc->exitCode(), proc->readAll().trimmed()};
    delete proc;
    return out;
}


void usbunmounter::start()
{
    //clearlist
    ui->mountlistview->clear();

    UID = runCmd("echo $UID").str;
    qDebug() << "UID is "<< UID;

    //get list of usb storage devices

    //first get list of mounted devices
    QStringList partitionlist;
    QStringList gvfslist;
    QString file_content;

    file_content = runCmd("df --local --output=source,target,size -H | grep /dev/").str;
    partitionlist = file_content.split("\n");
    qDebug() << "Partition list: " << partitionlist;
    //now build list for gvfs devices mtp and gphoto
    file_content = runCmd("ls -1 --color=never /run/user/" + UID +"/gvfs |grep mtp").str;
    file_content.append(runCmd("ls -1 --color=never /run/user/" + UID + "/gvfs |grep gphoto").str);
    gvfslist = file_content.split("\n");
    qDebug() << gvfslist;
    for (const QString &item : gvfslist) {
        QString dev = item;
        if ( dev == "") {
            qDebug() << "Dev" << dev;
        } else {
            qDebug() << "Dev" << dev;
            QString devpath = QString("/run/user/" + UID + "/"+ dev);
            qDebug() << "DevPath" << devpath;
            QString addtopartitionlist = QString(devpath + " " + item + " ");
            partitionlist << addtopartitionlist;
        }
    }

    qDebug() << "Partition list: " << partitionlist;



    // now check usb status of each item.  if usb is yes, add to usb list, if optical is yes, add to optical list

    bool isUSB;
    bool isCD;
    bool isMMC;
    bool isGPHOTO;
    bool isMTP;
    QString devicename;
    QListWidgetItem *list_item;
    QString point;
    QString size;
    QString label;
    QString partition;
    QString model;

    for (const QString &item : partitionlist) {
        //devicename = item.simplified().section(' ', 0 ,0).section('/', 2, 2);  //gives us device designation (sda, sdb, etc..)
        point = item.simplified().section(' ', 1, 1);
        size = item.simplified().section(' ', 2, 2);
        partition = item.simplified().section(' ', 0, 0);
        label = runCmd("udevadm info --query=property " + partition + " |grep ID_FS_LABEL=").str.section('=',1,1);
        qDebug() << "Mountpoint: " << point;
        qDebug() << "Size: " << size;
        qDebug() << "Partition: " << partition;
        qDebug() << "Label: " << label;
        //isUSB = system("udevadm info --query=property --path=/sys/block/" + devicename.toUtf8() + " | grep -qE '^DEVPATH=.*/usb[0-9]+/'") == 0;
        isUSB = system("udevadm info --query=property " + partition.toUtf8() + " | grep -qE '^DEVPATH=.*/usb[0-9]+/'") == 0;
        //isCD = system("udevadm info --query=property --path=/sys/block/" + devicename.toUtf8() + " | grep -q ID_TYPE=cd") == 0;
        isCD = system("udevadm info --query=property " + partition.toUtf8() + " | grep -q ID_TYPE=cd") == 0;
        isMMC = system("udevadm info --query=property " + partition.toUtf8() + " | grep -q ID_DRIVE_FLASH_SD=") == 0;
        if (point.section(':', 0, 0) == "gphoto2") {
            isGPHOTO=true;
        } else {
            isGPHOTO=false;
        }
        if (point.section(':', 0, 0) == "mtp") {
            isMTP=true;
        } else {
            isMTP=false;
        }

        //model = runCmd("udevadm info --query=property --path=/sys/block/" + devicename.toUtf8() + " | grep ID_MODEL=").str.section('=',1,1);

        model = runCmd("udevadm info --query=property " + partition.toUtf8() + " | grep ID_MODEL=").str.section('=',1,1);
        QString DEVTYPE = runCmd("udevadm info --query=property " + partition.toUtf8() + " |grep DEVTYPE=").str.section("/", -1, -1);
        //correction for partitionless disks, some devices like ereaders and some usb sticks don't have partitions
        if (DEVTYPE == "DEVTYPE=disk"){
            devicename = runCmd("udevadm info --query=property " + partition.toUtf8() + " |grep DEVPATH=").str.section("/", -1, -1);
        } else {
            devicename = runCmd("udevadm info --query=property " + partition.toUtf8() + " |grep DEVPATH=").str.section("/", -2, -2);
        }

        if (isGPHOTO || isMTP){
            model = point.section("=",1,1);
            devicename=model;
            isUSB=true;
            label="";
        }


        qDebug() << "Device name: " << devicename;
        qDebug() << "Model: " << model;
        qDebug() << "Is USB: " << isUSB;
        qDebug() << "Is CD: " << isCD;
        qDebug() << "Is MMC: " << isMMC;
        qDebug() << "Is MTP: " << isMTP;
        qDebug() << "Is GPHOTO: " << isGPHOTO;


        QString data;

        if (isUSB || isCD || isMMC) {
            list_item = new QListWidgetItem(ui->mountlistview);
            QString item2 = QString(model + " " + size + " " + tr("Volume").toUtf8() + " " + label);
            list_item->setText(item2);
            qDebug() << "widget item: " << item2;
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
            qDebug() << "Data list: " << data;
        }
    }

    if ( ui->mountlistview->count() > 0 ) {
        ui->mountlistview->item(0)->setSelected(true);
    } else {
        if (is_start) {
            list_item = new QListWidgetItem(ui->mountlistview);
            list_item->setText(tr("No Removable Device"));
            list_item->setIcon(QIcon::fromTheme("process-stop"));
            list_item->setData(Qt::UserRole, "none");
        } else {
            qApp->quit();
        }
    }
}
usbunmounter::~usbunmounter()
{
    delete ui;
}


void usbunmounter::on_mountlistview_itemActivated(QListWidgetItem *item)
{
    //if device is mmc, just unmount.if usb, also poweroff. if device is cd/dvd, eject as well, then remove list item
    //if no device, then exit
    QString cmd;
    QString cmd2;
    QString cmd3;
    QString cmd4;
    QString point = QString(item->text());
    qDebug() << "clicked mount point" << point;
    cmd2 = tr("Unmounting " + point.toUtf8());
    cmd3 = tr(point.toUtf8() + " is Safe to Remove");
    cmd4 = tr("Other partitions still mounted on device");
    QString title = tr("MX USB Unmounter");
    QString type = item->data(Qt::UserRole).toString().section(";", 0, 0);
    qDebug() << item->data(Qt::UserRole).toString();
    qDebug() << "type is" << type;

    if (item->data(Qt::UserRole).toString() == "none") {
        qApp->quit();
        return;
    }
    // run operation on selected device

    QString mountdevice = item->data(Qt::UserRole).toString().section(";", 2, 2);
    QString partitiondevice = item->data(Qt::UserRole).toString().section(";", 1, 1);
    QString model = item->data(Qt::UserRole).toString().section(";", 3, 3);
    qDebug() << "Mount device is" << mountdevice;
    qDebug() << "Partion device is" << partitiondevice;
    qDebug() << type;
    int out;
    QString out2;

    // if item is mmc, unmount only, don't "eject"
    if (type == "mmc") {
        out = runCmd("umount " + partitiondevice).exit_code;
    }
    if (type == "usb") {
        out = runCmd("umount /dev/" + mountdevice + "?*").exit_code;
        qDebug() << "unmount paritions exit code" << out;
        if (out != 0 ) {
            //try just unmounting the device
            out = runCmd("umount /dev/" + mountdevice).exit_code;
            qDebug() << "unmount device exit code" << out;
            if (out != 0 ) {
                out2 = runCmd("cat /etc/mtab | grep -q " + mountdevice + " && echo $?").str;
                qDebug() << "out2 is " << out2;
                if (out2 == "") {
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
        out = runCmd("gvfs-mount -u /run/user/$UID/gvfs/" + mountdevice).exit_code;
    }

    qDebug() << "out is "<< out;

        if (out == 0) {
        // if device is usb, go ahead and power off "eject" and notify user
        if ( type == "usb" ) {
            system("udisksctl power-off -b /dev/" + mountdevice.toUtf8());
            system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd2.toUtf8() + "'");
            system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd3.toUtf8() + "'");
        }
        // if device is mtp or gphoto2, say unmount was a success
        if (type == "mtp" || type  == "gphoto2") {
            system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd2.toUtf8() + "'");
            system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd3.toUtf8() + "'");
        }
        // if device is CD, done, and notify user of success
        if ( type == "cd" ) {
            system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd2.toUtf8() + "'");
            system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd3.toUtf8() + "'");
        }

        // if device is mmc, check for additional mounted partitions.  If none, say safe to eject.  If more found, say so.
        if ( type == "mmc") {
            QString mmc_check;
            mmc_check = runCmd("df --local --output=source,target,size -H | grep " + mountdevice).str;
            if (mmc_check ==  "" ) {
                system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd2.toUtf8() + "'");
                system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd3.toUtf8() + "'");
            } else {
                system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd2.toUtf8() + "'");
                system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd4.toUtf8() + " " + model.toUtf8() + "'");
            }
        }

    } else {  //if umount operation failed, say so
        qDebug() << "Warning";
        cmd3 = tr("Unable to  Unmount, Device in Use");
        system("notify-send -i drive-removable-media '" + title.toUtf8() + "' '" + cmd3.toUtf8() + "'");
    }
    //flag for first time run
    is_start = false;
    start();
}

// implement change event that closes app when window loses focus
void usbunmounter::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange) {
        if(this->isActiveWindow()) {
            qDebug() << "focusinEvent";
        } else {
            qDebug() << "focusOutEvent";
            qApp->quit();
        }
    }
}

void usbunmounter::on_cancel_pressed()
{
    qApp->quit();
}

// process keystrokes
void usbunmounter::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        qApp->quit();
    }
}

// About mx-usb-unmounter
int usbunmounter::about()
{
    QString version = runCmd("dpkg-query --show mx-usb-unmounter").str.simplified().section(' ',1,1);
    QMessageBox msgBox(QMessageBox::NoIcon,
                       tr("About MX USB Unmounter"), "<p align=\"center\"><b><h2>" +
                       tr("MX USB Unmounter") + "</h2></b></p><p align=\"center\">" + tr("Version: ") + version + "</p><p align=\"center\"><h3>" +
                       tr("Quickly Unmount Removable Media") +
                       "</h3></p><p align=\"center\"><a href=\"http://mxlinux.org\">http://mxlinux.org</a><br /></p><p align=\"center\">" +
                       tr("Copyright (c) MX Linux") + "<br /><br /></p>", 0, 0);
    msgBox.addButton(tr("Cancel"), QMessageBox::AcceptRole); // because we want to display the buttons in reverse order we use counter-intuitive roles.
    msgBox.addButton(tr("License"), QMessageBox::RejectRole);
    if (msgBox.exec() == QMessageBox::RejectRole) {
        system("mx-viewer file:///usr/share/doc/mx-usb-unmounter/license.html '" + tr("MX USB Unmounter").toUtf8() + " " + tr("License").toUtf8() + "'");
    }
    return 1;
}
