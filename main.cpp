#include "mainwindow.h"
#include <QApplication>
#include <QLocale>
#include <QLockFile>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString user = QProcessEnvironment::systemEnvironment().value("USER");
    QLockFile lockfile("/var/lock/mx-usb-unmounter_" + user + ".lock");
    if (!lockfile.tryLock()) {
        return EXIT_FAILURE;
    }

    QTranslator qtTran;
    qtTran.load(QString("qt_") + QLocale::system().name());
    QApplication::installTranslator(&qtTran);

    QTranslator appTran;
    appTran.load(QString("mx-usb-unmounter_") + QLocale::system().name(), "/usr/share/mx-usb-unmounter/locale");
    QApplication::installTranslator(&appTran);

    MainWindow w(argv[1]);
    w.hide();
    return QApplication::exec();
}
