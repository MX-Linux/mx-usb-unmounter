#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDialog>
#include <QListWidgetItem>
#include <QMenu>
#include <QProcess>
#include <QSystemTrayIcon>

namespace Ui {
class MainWindow;
}

// struct for outputing both the exit code and the strings when running a command
struct Output {
    int exit_code;
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
    void changeEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void on_cancel_pressed();
    void on_mountlistview_itemActivated(QListWidgetItem *item);
    void iconActivated(QSystemTrayIcon::ActivationReason reason);


private:
    Ui::MainWindow *ui;
    QAction *aboutAction{};
    QAction *helpAction{};
    QAction *listDevicesAction{};
    QAction *quitAction{};
    QAction *toggleAutostartAction{};
    QMenu *menu{};
    QProcess proc;
    QSystemTrayIcon *trayIcon{};

    void createActions();
    void createMenu();
    static void help();
    void setPosition();
    static void toggleAutostart();

};

#endif // MAINWINDOW_H
