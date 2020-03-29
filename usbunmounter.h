#ifndef USBUNMOUNTER_H
#define USBUNMOUNTER_H

#include <QDialog>
#include <QProcess>
#include <QListWidgetItem>

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
    int about();
    Output runCmd(QString cmd);
    bool is_start;
    QString UID;

private slots:
    void on_mountlistview_itemActivated(QListWidgetItem *item);
    void keyPressEvent(QKeyEvent *event);
    void changeEvent(QEvent *event);
    void on_cancel_pressed();


private:
    Ui::usbunmounter *ui;

};

#endif // USBUNMOUNTER_H
