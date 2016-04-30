#include "usbunmounter.h"
#include <QApplication>
#include <QTranslator>
#include <QLocale>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator qtTran;
    qtTran.load(QString("qt_") + QLocale::system().name());
    a.installTranslator(&qtTran);

    QTranslator appTran;
    appTran.load(QString("mx-usb-unmounter_") + QLocale::system().name(), "/usr/share/mx-usb-unmounter/locale");
    a.installTranslator(&appTran);

    QString arg1;
    arg1 = argv[1];
    //arg1 = "--help";

    usbunmounter w(arg1);
    w.show();

    return a.exec();

}
