#include "mainwindow.h"
#include <QApplication>
#include <QLocale>
#include <QLockFile>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    const QString user = QProcessEnvironment::systemEnvironment().value("USER");
    QLockFile lockfile(QString("/var/lock/mx-usb-unmounter_%1.lock").arg(user));
    if (!lockfile.tryLock()) {
        qWarning("Another instance is already running.");
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
