#include "clipboardmanager.h"
#include <QApplication>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include "qencryptrc4.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("Tekzip");
    QCoreApplication::setOrganizationDomain("tekzip.me");
    QCoreApplication::setApplicationName("SendClip");

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(NULL, QObject::tr("Systry"), QObject::tr("I couldn't detect any system try on this system."));
        return 1;
    }
    QApplication::setQuitOnLastWindowClosed(false);

    ClipboardManager manager;
    manager.createTrayIcon();

    return a.exec();
}
