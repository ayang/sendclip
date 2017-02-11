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
    qencryptrc4.cpp \
    aboutdialog.cpp

HEADERS  += settingsdialog.h \
    clipboardmanager.h \
    qencryptrc4.h \
    aboutdialog.h

FORMS    += settingsdialog.ui \
    aboutdialog.ui

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

# qhttpserver
#win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../qhttpserver/lib/release/ -lqhttpserver
#else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../qhttpserver/lib/debug/ -lqhttpserver
#else:unix: LIBS += -L$$PWD/../qhttpserver/lib/ -lqhttpserver

QHTTPDIR = $$PWD/../qhttp
INCLUDEPATH +=  . $$QHTTPDIR/src
LIBS        += -L$$QHTTPDIR/xbin -lqhttp

unix {
    CONFIG += link_pkgconfig
}

#USE_APP_INDICATOR = 1

defined(USE_APP_INDICATOR, var) {
    message("use app indicator")
    DEFINES += USE_APP_INDICATOR
    HEADERS += vendor/qappindicator.h
    SOURCES += vendor/qappindicator.cc
}
