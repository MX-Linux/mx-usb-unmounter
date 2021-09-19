#ifndef USBUNMOUNTER_H
#define USBUNMOUNTER_H

#include <QDialog>
#include <QListWidgetItem>
#include <QMenu>
#include <QProcess>
#include <QSystemTrayIcon>

namespace Ui {
class usbunmounter;
}

// struct for outputing both the exit code and the strings when running a command
struct Output {
    int exit_code;
    QString str;
};

class usbunmounter : public QDialog
{
    Q_OBJECT

public:
    usbunmounter(QString arg1, QWidget *parent = nullptr);
    ~usbunmounter();
    void start();
    void about();
    Output runCmd(QString cmd);
    QString UID;

private slots:
    void changeEvent(QEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void on_cancel_pressed();
    void on_mountlistview_itemActivated(QListWidgetItem *item);
    void iconActivated(QSystemTrayIcon::ActivationReason reason);


private:
    Ui::usbunmounter *ui;
    QAction *aboutAction;
    QAction *helpAction;
    QAction *listDevicesAction;
    QAction *quitAction;
    QAction *toggleAutostartAction;
    QMenu *menu;
    QSystemTrayIcon *trayIcon;

    void createActions();
    void createMenu();
    void help();
    void toggleAutostart();

};

#endif // USBUNMOUNTER_H
