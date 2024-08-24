#pragma once

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDialog>
#include <QListWidgetItem>
#include <QMenu>
#include <QProcess>
#include <QSettings>
#include <QSystemTrayIcon>

namespace Ui
{
class MainWindow;
}

// struct for outputing both the exit code and the strings when running a command
struct Output {
    int exitCode;
    QString str;
};

class MainWindow : public QDialog
{
    Q_OBJECT

public:
    explicit MainWindow(const QString &arg1, QWidget *parent = nullptr);
    ~MainWindow() override;
    void about();
    void listDevices();
    void start();
    Output runCmd(const QString &cmd);
    QString UID;

private slots:
    void cancelPressed();
    void changeEvent(QEvent *event) override;
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void keyPressEvent(QKeyEvent *event) override;
    void mountlistviewItemActivated(QListWidgetItem *item);
    void onInterfacesRemoved(const QDBusMessage &message);
    void toggleAutostart();
    void toggleHideIcon();

private:
    Ui::MainWindow *ui;
    QAction *aboutAction {};
    QAction *helpAction {};
    QAction *listDevicesAction {};
    QAction *quitAction {};
    QAction *toggleAutostartAction {};
    QAction *toggleHideAction {};
    QDBusConnection *systemBus {};
    QMenu *menu {};
    QProcess proc;
    QSettings settings;
    QSystemTrayIcon *trayIcon {};
    const QString serviceName = "org.freedesktop.UDisks2";
    const QString objectPath = "/org/freedesktop/UDisks2";
    const QString interfaceName = "org.freedesktop.DBus.ObjectManager";

    bool hasDevices();
    static void help();
    void createActions();
    void createMenu();
    void disconnectFromDBus();
    void setPosition();
    void deviceMonitor();
};
