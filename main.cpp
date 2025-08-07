#include <QApplication>
#include <QLocale>
#include <QLockFile>
#include <QTranslator>

#include "mainwindow.h"
#include "version.h"

int main(int argc, char *argv[])
{
    // Set Qt platform to XCB (X11) if not already set and we're in X11 environment
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        if (!qEnvironmentVariableIsEmpty("DISPLAY")) {
            qputenv("QT_QPA_PLATFORM", "xcb");
        }
    }

    QApplication app(argc, argv);

    const QString user = QProcessEnvironment::systemEnvironment().value("USER");
    QLockFile lockfile(QString("/var/lock/mx-usb-unmounter_%1.lock").arg(user));
    if (!lockfile.tryLock()) {
        qWarning("Another instance is already running.");
        return EXIT_FAILURE;
    }

    QApplication::setWindowIcon(QIcon::fromTheme(QApplication::applicationName()));
    QApplication::setOrganizationName("MX-Linux");
    QApplication::setApplicationVersion(VERSION);

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
