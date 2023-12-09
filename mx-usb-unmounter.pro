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


SOURCES += main.cpp \
    about.cpp \
    mainwindow.cpp

HEADERS  += \
    about.h \
    mainwindow.h

FORMS    += \
    mainwindow.ui

TRANSLATIONS += translations/mx-usb-unmounter_en.ts 

CONFIG += release warn_on thread qt c++11
