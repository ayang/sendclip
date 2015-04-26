#ifndef CLIPBOARDMANAGER_H
#define CLIPBOARDMANAGER_H

#include <QObject>
#include <QtWidgets>

class SettingsDialog;
class QUdpSocket;

class ClipboardManager : public QObject
{
    Q_OBJECT
public:
    explicit ClipboardManager(QObject *parent = 0);
    ~ClipboardManager();

signals:

public slots:
    void sendClipboard();
    void reciveData();
    void showSettingsDialog();
    void quit();
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void createTrayIcon();
    void reload();

private:
    void createTrayMenu();

private:
    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;

    QAction *sendClipAction;
    QAction *settingsAction;
    QAction *quitAction;

    SettingsDialog *settingsDialog;

    QUdpSocket *udpSocket;
    QStringList sendAddress;
    uint port;
    QString username;
    QString key;
    QByteArray buffer;
};

#endif // CLIPBOARDMANAGER_H
