#include "clipboardmanager.h"
#include <QApplication>
#include <QNetworkProxyFactory>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include "qencryptrc4.h"

#ifdef USE_APP_INDICATOR
#include "vendor/qappindicator.h"

extern "C" {

void on_menu_main_win_trigger(GtkMenu*, gpointer data);

void on_menu_exit_trigger(GtkMenu*, gpointer data);

}
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("Tekzip");
    QCoreApplication::setOrganizationDomain("tekzip.me");
    QCoreApplication::setApplicationName("SendClip");

    // set no proxy to fix crash when system proxy is using pac
    QNetworkProxyFactory::setUseSystemConfiguration(false);

#ifndef USE_APP_INDICATOR
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(NULL, QObject::tr("Systry"), QObject::tr("I couldn't detect any system try on this system."));
        return 1;
    }
    QApplication::setQuitOnLastWindowClosed(false);

    ClipboardManager manager;
    manager.createTrayIcon();
#else
    QAppIndicator *indicator;
    indicator = new QAppIndicator(QStringLiteral("ExampleApp"),
            QStringLiteral("clock"),
            APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
    indicator->setStatus(APP_INDICATOR_STATUS_ACTIVE);
    indicator->setAttentionIconFull(
            QStringLiteral("system-hibernate"),
            QStringLiteral("window showed"));
    indicator->setMenu(buildIndicatorMenu());
#endif

    return a.exec();
}

#ifdef USE_APP_INDICATOR
GtkMenu *Application::buildIndicatorMenu()
{
    auto menu = gtk_menu_new();

    auto item = gtk_menu_item_new_with_label("Show Window");
    g_signal_connect(item, "activate", G_CALLBACK(on_menu_main_win_trigger), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    // seperator
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_menu_item_new());

    item = gtk_menu_item_new_with_label("Exit");
    g_signal_connect(item, "activate", G_CALLBACK(on_menu_exit_trigger), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    return GTK_MENU(menu);
}

void on_menu_main_win_trigger(GtkMenu*, gpointer data)
{
//	auto app = static_cast<Application*>(data);
//	app->showMainWindow();
}

void on_menu_exit_trigger(GtkMenu*, gpointer data)
{
    auto app = static_cast<Application*>(data);
    app->quit();
}
#endif
