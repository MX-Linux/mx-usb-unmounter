#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QLockFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString user = QProcessEnvironment::systemEnvironment().value("USER");
    QLockFile lockfile("/var/lock/mx-usb-unmounter_" + user + ".lock");
    if (!lockfile.tryLock())
        return EXIT_FAILURE;

    QTranslator qtTran;
    qtTran.load(QString("qt_") + QLocale::system().name());
    a.installTranslator(&qtTran);

    QTranslator appTran;
    appTran.load(QString("mx-usb-unmounter_") + QLocale::system().name(), "/usr/share/mx-usb-unmounter/locale");
    a.installTranslator(&appTran);

    MainWindow w(argv[1]);
    w.hide();
    return a.exec();
}
