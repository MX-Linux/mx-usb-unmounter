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
    qDebug() << "Partition list: " << partitionlist;

    // now check usb status of each item.  if usb is yes, add to usb list, if optical is yes, add to optical list

    bool isUSB;
    bool isCD;
    QString devicename;
    QString item;
    QListWidgetItem *list_item;

    foreach (item, partitionlist) {
        devicename = item.mid(5,3);  //gives us device designation (sda, sdb, etc..)
        qDebug() << "Device name: " << devicename;
        isUSB = system("udevadm info --query=property --path=/sys/block/" + devicename.toUtf8() + " | grep -q ID_BUS=usb") == 0;
        isCD = system("udevadm info --query=property --path=/sys/block/" + devicename.toUtf8() + " | grep -q ID_TYPE=cd") == 0;
        qDebug() << "Is USB: " << isUSB;
        qDebug() << "Is CD: " << isCD;
        if (isUSB || isCD ) {
            QString item2 = runCmd("df " + item + " --output=target |grep /").str;
            qDebug() << "widget item: " << item2;
            list_item = new QListWidgetItem(ui->mountlistview);
            list_item->setText(item2);
            if (isUSB) {
                list_item->setIcon(QIcon::fromTheme("drive-removable-media"));
                list_item->setData(Qt::UserRole, "usb");
            }
            if (isCD) {
                list_item->setIcon(QIcon::fromTheme("drive-optical"));
                list_item->setData(Qt::UserRole, "cd");
            }
        }
    }
    this->move(QCursor::pos());
    if ( ui->mountlistview->count() > 0 ) {
        ui->mountlistview->item(0)->setSelected(true);
    }
}

usbunmounter::~usbunmounter()
{
    delete ui;
}


void usbunmounter::on_mountlistview_itemActivated(QListWidgetItem *item)
{
    //if device is usb, just unmount. if device is cd/dvd, eject as well, then remove list item
    QString cmd;
    QString point = QString(item->text());
    qDebug() << "clicked mount point" << point;

    if (item->data(Qt::UserRole).toString() == "usb") {
        cmd = "umount";
    } else {
        cmd = "eject";
    }

    // run operation.  if exit is unsuccessful, state device is busy
    int out = runCmd(cmd + " '" + point + "'").exit_code;
    qDebug() << out;
    if (out == 0) {
        item->~QListWidgetItem();
    } else {
        this->setVisible(false);
        qDebug() << "Warning";
        QMessageBox msgBox(QMessageBox::Warning,
                           tr("Warning"), "<p align=\"center\"><h3>" + tr("Device Busy:") + " " + point + "</h4></p><p align=\"center\">", 0, this);
        msgBox.addButton(tr("OK"), QMessageBox::AcceptRole);
        if (msgBox.exec() == QMessageBox::AcceptRole) {
            this->setVisible(true);
        }
    }
}
void usbunmounter::on_cancel_pressed()
{
    exit(0);
}

// process keystrokes
void usbunmounter::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
        exit(0);
}

