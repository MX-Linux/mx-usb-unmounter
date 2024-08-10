#pragma once

#include <QDialog>
#include <QListWidgetItem>
#include <QMenu>
#include <QProcess>
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
    void start();
    void about();
    Output runCmd(const QString &cmd);
    QString UID;

private slots:
    void cancelPressed();
    void changeEvent(QEvent *event) override;
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void keyPressEvent(QKeyEvent *event) override;
    void mountlistviewItemActivated(QListWidgetItem *item);

private:
    Ui::MainWindow *ui;
    QAction *aboutAction {};
    QAction *helpAction {};
    QAction *listDevicesAction {};
    QAction *quitAction {};
    QAction *toggleAutostartAction {};
    QMenu *menu {};
    QProcess proc;
    QSystemTrayIcon *trayIcon {};

    static void help();
    void createActions();
    void createMenu();
    void setPosition();
    void toggleAutostart();
};
