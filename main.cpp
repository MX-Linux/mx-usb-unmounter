#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QLockFile>
#include <QStandardPaths>
#include <QTranslator>

#include "mainwindow.h"

#ifndef VERSION
    #define VERSION "?.?.?.?"
#endif

static void installTranslations(QTranslator &qtTran, QTranslator &appTran)
{
    const QString qtDir = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    if (qtTran.load(QStringLiteral("qt_") + QLocale::system().name(), qtDir)
        || qtTran.load(QStringLiteral("qtbase_") + QLocale::system().name(), qtDir)) {
        QCoreApplication::installTranslator(&qtTran);
    }

    if (appTran.load(QString("mx-usb-unmounter_") + QLocale::system().name(), "/usr/share/mx-usb-unmounter/locale")) {
        QCoreApplication::installTranslator(&appTran);
    }
}

int main(int argc, char *argv[])
{
    const QByteArray option = argc == 2 ? QByteArray(argv[1]) : QByteArray();
    const bool headless = qEnvironmentVariableIsEmpty("DISPLAY") && qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")
                          && qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM");
    if (option == "--help-all" || (headless && (option == "-h" || option == "--help"))) {
        QCoreApplication app(argc, argv);
        QTranslator qtTran;
        QTranslator appTran;
        installTranslations(qtTran, appTran);
        QCommandLineParser parser;
        parser.setApplicationDescription(MainWindow::tr("Quickly Unmount Removable Media"));
        parser.addHelpOption();
        parser.process(app);
        return EXIT_SUCCESS;
    }

    if (headless) {
        qWarning("mx-usb-unmounter: no display available (DISPLAY and WAYLAND_DISPLAY are both unset); "
                "a graphical session is required to run this program.");
        return EXIT_FAILURE;
    }

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
    QTranslator appTran;
    installTranslations(qtTran, appTran);

    MainWindow w(argv[1]);
    w.hide();
    return QApplication::exec();
}
