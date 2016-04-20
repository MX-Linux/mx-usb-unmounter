#include "usbunmounter.h"
#include "ui_usbunmounter.h"
#include <QDebug>

usbunmounter::usbunmounter(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::usbunmounter)
{
    ui->setupUi(this);
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
    //get list of usb storage devices

    //first get list of mounted devices
    QStringList partitionlist;
    QString file_content;
    file_content = runCmd("df --local --output=source |grep /dev/").str;
    partitionlist = file_content.split("\n");
    //  qDebug() << partitionlist;

    // now check usb status of each item.  if usb is yes, add to usb list, if optical is yes, add to optical list

    QString devicename;
    QString item;\
    QStringList mountlist;
    QStringList mountlistoptical;

    foreach (item, partitionlist) {
        devicename = item.mid(5,3);  //gives us device designation (sda, sdb, etc..)
        //   qDebug() << devicename;
        QString usbcheck = runCmd("udevadm info --query=property --path=/sys/block/" + devicename + " | grep ID_BUS=usb |cut -d '=' -f2").str;
        //   qDebug() << usbcheck;
        if ( usbcheck == "usb") {
            QString item2 = runCmd("df " + item + " --output=target |grep /").str;
            //     qDebug() << item2;
            mountlist.append(item2);
        }
        QString opticalcheck = runCmd("udevadm info --query=property --path=/sys/block/" + devicename + " | grep ID_TYPE=cd |cut -d '=' -f2").str;
        if ( opticalcheck =="cd") {
            QString item2 = runCmd("df " + item + " --output=target |grep /").str;
            mountlistoptical.append(item2);
        }
    }
    //    qDebug() << mountlist;
    //    qDebug() << mountlistoptical;

    //build list for view , list be mountpoint
    // use meta data to track device type
    QVariant devicetype;

    //if device is usb, add to mountlist, create list entry, assing metadata (usb)
    QListWidgetItem *list_item;
    foreach (item, mountlist) {
        devicetype = QString("usb");
        list_item = new QListWidgetItem(ui->mountlistview);
        list_item->setText(item);
        list_item->setIcon(QIcon::fromTheme("drive-removable-media"));
        list_item->setData(Qt::UserRole, devicetype);
    }

    //if device is CD/DVD, add to mountlistoptical, create list entry assign metadata (cd)
    foreach (item, mountlistoptical) {
        devicetype = QString("cd");
        list_item = new QListWidgetItem(ui->mountlistview);
        list_item->setText(item);
        list_item->setIcon(QIcon::fromTheme("drive-optical"));
        list_item->setData(Qt::UserRole, devicetype);

    }
    this->move(QCursor::pos());
}


usbunmounter::~usbunmounter()
{
    delete ui;
}


void usbunmounter::on_mountlistview_itemDoubleClicked(QListWidgetItem *item)
{
    //if device is usb, just unmount. if device is cd/dvd, eject as well, then remove list item

    if (item->data(Qt::UserRole).toString() == "usb") {
        QString point = QString(item->text());
        runCmd("umount " + point);
        item->~QListWidgetItem();
    }
    if (item->data(Qt::UserRole).toString() == "cd" ) {
        QString point = QString(item->text());
        runCmd("eject " + point);
        item->~QListWidgetItem();
    }
}
