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
    explicit usbunmounter(QWidget *parent = 0);
    ~usbunmounter();
    void start();
    Output runCmd(QString cmd);



private slots:
    void on_mountlistview_itemDoubleClicked(QListWidgetItem *item);

private:
    Ui::usbunmounter *ui;
};

#endif // USBUNMOUNTER_H
