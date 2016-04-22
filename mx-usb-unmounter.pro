#-------------------------------------------------
#
# Project created by QtCreator 2016-04-19T14:58:51
#
#-------------------------------------------------

#DEFINES += QT_NO_WARNING_OUTPUT QT_NO_DEBUG_OUTPUT

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = mx-usb-unmounter
TEMPLATE = app


SOURCES += main.cpp\
        usbunmounter.cpp

HEADERS  += usbunmounter.h

FORMS    += usbunmounter.ui

TRANSLATIONS += translations/mx-usb-unmounter_ca.ts \
                translations/mx-usb-unmounter_de.ts \
                translations/mx-usb-unmounter_el.ts \
                translations/mx-usb-unmounter_es.ts \
                translations/mx-usb-unmounter_fr.ts \
                translations/mx-usb-unmounter_it.ts \
                translations/mx-usb-unmounter_ja.ts \
                translations/mx-usb-unmounter_nl.ts \
                translations/mx-usb-unmounter_sv.ts
