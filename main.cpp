#include <QApplication>
#include <QDir>
#include <QLocale>
#include <QLockFile>
#include <QStandardPaths>
#include <QTranslator>

#include "mainwindow.h"

#ifndef VERSION
    #define VERSION "?.?.?.?"
#endif

int main(int argc, char *argv[])
{
    // Set Qt platform to XCB (X11) if not already set and we're in X11 environment
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        if (!qEnvironmentVariableIsEmpty("DISPLAY") && qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
            qputenv("QT_QPA_PLATFORM", "xcb");
        }
    }

    QApplication app(argc, argv);

    const QString user = QProcessEnvironment::systemEnvironment().value("USER");
    const QString runtimeDir = qEnvironmentVariableIsEmpty("XDG_RUNTIME_DIR")
                                   ? QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                                   : qEnvironmentVariable("XDG_RUNTIME_DIR");
    QDir().mkpath(runtimeDir);
    const QString lockPath = QString("%1/mx-usb-unmounter_%2.lock").arg(runtimeDir, user);
    QLockFile lockfile(lockPath);
    if (!lockfile.tryLock()) {
        qint64 pid = -1;
        QString host;
        QString appName;
        const bool hasInfo = lockfile.getLockInfo(&pid, &host, &appName);
        qWarning() << "Another instance is already running. Lock file:" << lockPath
                   << (hasInfo ? QStringLiteral("Owner PID: %1").arg(pid)
                               : QStringLiteral("Owner PID: unknown"))
                   << (hasInfo ? QStringLiteral("Host: %1").arg(host)
                               : QStringLiteral("Host: unknown"))
                   << (hasInfo ? QStringLiteral("App: %1").arg(appName)
                               : QStringLiteral("App: unknown"));
        return EXIT_FAILURE;
    }

    QApplication::setWindowIcon(QIcon::fromTheme(QApplication::applicationName()));
    QApplication::setOrganizationName("MX-Linux");
    QApplication::setApplicationVersion(VERSION);

    QTranslator qtTran;
    if (qtTran.load(QString("qt_") + QLocale::system().name())) {
        QApplication::installTranslator(&qtTran);
    }

    QTranslator appTran;
    if (appTran.load(QString("mx-usb-unmounter_") + QLocale::system().name(), "/usr/share/mx-usb-unmounter/locale")) {
        QApplication::installTranslator(&appTran);
    }

    MainWindow w(argv[1]);
    w.hide();
    return QApplication::exec();
}
