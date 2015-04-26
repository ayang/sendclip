#-------------------------------------------------
#
# Project created by QtCreator 2015-04-25T15:59:50
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sendclip
TEMPLATE = app


SOURCES += main.cpp\
        settingsdialog.cpp \
    clipboardmanager.cpp \
    qencryptrc4.cpp

HEADERS  += settingsdialog.h \
    clipboardmanager.h \
    qencryptrc4.h

FORMS    += settingsdialog.ui

RESOURCES += \
    images.qrc

DISTFILES += \
    Info.plist \
    sendclip.rc \
    images/main.icns \
    images/main.ico

QMAKE_INFO_PLIST = Info.plist
ICON = images/main.icns
RC_FILE = sendclip.rc
