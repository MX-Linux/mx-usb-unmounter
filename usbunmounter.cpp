#include "usbunmounter.h"
#include "ui_usbunmounter.h"
#include <QDebug>
#include <QKeyEvent>
#include <QMessageBox>

usbunmounter::usbunmounter(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::usbunmounter)
{
    ui->setupUi(this);
    this->setWindowIcon(QIcon::fromTheme("drive-removable-media"));
    this->setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
    this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    this->move(QCursor::pos());
    is_start = true;
    start();
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

    //get list of usb storage devices

    //first get list of mounted devices
    QStringList partitionlist;
    QString file_content;
    file_content = runCmd("df --local --output=source,target,size -H | grep /dev/").str;
    partitionlist = file_content.split("\n");
    qDebug() << "Partition list: " << partitionlist;

    // now check usb status of each item.  if usb is yes, add to usb list, if optical is yes, add to optical list

    bool isUSB;
    bool isCD;
    bool isMMC;
    QString devicename;
    QString item;
    QListWidgetItem *list_item;
    QString point;
    QString size;
    QString label;
    QString partition;
    QString model;

    foreach (item, partitionlist) {
        //devicename = item.simplified().section(' ', 0 ,0).section('/', 2, 2);  //gives us device designation (sda, sdb, etc..)
        point = item.simplified().section(' ', 1, 1);
        size = item.simplified().section(' ', 2, 2);
        partition = item.simplified().section(' ', 0, 0);
        label = runCmd("udevadm info --query=property " + partition + " |grep ID_FS_LABEL=").str.section('=',1,1);
        qDebug() << "Mountpoint: " << point;
        qDebug() << "Size: " << size;
        qDebug() << "Partition: " << partition;
        qDebug() << "Label: " << label;
        //isUSB = system("udevadm info --query=property --path=/sys/block/" + devicename.toUtf8() + " | grep -q ID_BUS=usb") == 0;
        isUSB = system("udevadm info --query=property " + partition.toUtf8() + " | grep -q ID_BUS=usb") == 0;
        //isCD = system("udevadm info --query=property --path=/sys/block/" + devicename.toUtf8() + " | grep -q ID_TYPE=cd") == 0;
        isCD = system("udevadm info --query=property " + partition.toUtf8() + " | grep -q ID_TYPE=cd") == 0;
        isMMC = system("udevadm info --query=property " + partition.toUtf8() + " | grep -q ID_DRIVE_FLASH_SD=") == 0;
        //model = runCmd("udevadm info --query=property --path=/sys/block/" + devicename.toUtf8() + " | grep ID_MODEL=").str.section('=',1,1);
        model = runCmd("udevadm info --query=property " + partition.toUtf8() + " | grep ID_MODEL=").str.section('=',1,1);
        devicename = runCmd("udevadm info --query=property " + partition.toUtf8() + " |grep DEVPATH=").str.section("/", -2, -2);

        qDebug() << "Device name: " << devicename;
        qDebug() << "Model: " << model;
        qDebug() << "Is USB: " << isUSB;
        qDebug() << "Is CD: " << isCD;
        qDebug() << "Is MMC: " << isMMC;


        QString data;

        if (isUSB || isCD || isMMC) {
            list_item = new QListWidgetItem(ui->mountlistview);
            QString item2 = QString(model + " " + size + " " + tr("Volume").toUtf8() + " " + label);
            list_item->setText(item2);
            qDebug() << "widget item: " << item2;
            QString data;
            if (isUSB) {
                list_item->setIcon(QIcon::fromTheme("drive-removable-media"));
                data = QString("usb:" + partition + ":" + devicename + ":" + model);
                list_item->setData(Qt::UserRole, data);
                list_item->setToolTip(point);
            }
            if (isCD) {
                list_item->setIcon(QIcon::fromTheme("drive-optical"));
                data = QString("cd:" + partition + ":" + devicename + ":" + model);
                list_item->setData(Qt::UserRole, data);
                list_item->setToolTip(point);
            }
            if (isMMC) {
                list_item->setIcon(QIcon::fromTheme("media-flash"));
                data = QString("mmc:" + partition + ":" + devicename + ":" + model);
                list_item->setData(Qt::UserRole, data);
                list_item->setToolTip(point);
            }
        }
    }

    if ( ui->mountlistview->count() > 0 ) {
        ui->mountlistview->item(0)->setSelected(true);
    } else {
        if (is_start) {
            list_item = new QListWidgetItem(ui->mountlistview);
            list_item->setText(tr("No Removable Device"));
            list_item->setIcon(QIcon::fromTheme("gtk-cancel"));
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
    cmd3 = tr("Your Device is Safe to Remove");
    cmd4 = tr("Other partitions still mounted on device");
    QString title = tr("MX USB Unmounter");
    QString type = item->data(Qt::UserRole).toString().section(":", 0, 0);
    qDebug() << item->data(Qt::UserRole).toString();
    qDebug() << "type is" << type;

    if (item->data(Qt::UserRole).toString() == "none") {
        qApp->quit();
    }
    // run operation.  if exit is unsuccessful, state device is busy via notify-send

    QString mountdevice = item->data(Qt::UserRole).toString().section(":", 2, 2);
    QString partitiondevice = item->data(Qt::UserRole).toString().section(":", 1, 1);
    QString model = item->data(Qt::UserRole).toString().section(":", 3, 3);
    qDebug() << "Mount device is" << mountdevice;
    qDebug() << "Partion device is" << partitiondevice;
    qDebug() << type;
    int out;

    // if item is mmc, unmount only, don't "eject"
    if (type == "mmc") {
        out = runCmd("umount " + partitiondevice).exit_code;
    }
    if (type == "usb") {
        out = runCmd("umount /dev/" + mountdevice + "?*").exit_code;
    }
    if (type == "cd") {
        out = runCmd("eject " + partitiondevice).exit_code;
    }

    qDebug() << out;
    if (out == 0) {
        // if device is usb, go ahead and power off "eject" and notify user
        if ( type == "usb" ) {
            system("udisksctl power-off -b /dev/" + mountdevice.toUtf8());
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


