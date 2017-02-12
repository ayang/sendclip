#ifndef CLIPBOARDMANAGER_H
#define CLIPBOARDMANAGER_H

#include <QObject>
#include <QtWidgets>

namespace qhttp {
    namespace server {
        class QHttpRequest;
        class QHttpResponse;
    }
}

using namespace qhttp::server;

class SettingsDialog;
class QUdpSocket;
class QNetworkAccessManager;
class QNetworkReply;

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
    void about();
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void createTrayIcon();
    void reload();
    void handleHttp(QHttpRequest *req, QHttpResponse *resp);
    void getTextFinish();
    void getHtmlFinish();
    void getImageFinish();

private:
    void createTrayMenu();
    QByteArray encrypt(const QByteArray &data);
    QByteArray decrypt(const QByteArray &data);

private:
    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;

    QAction *sendClipAction;
    QAction *settingsAction;
    QAction *quitAction;
    QAction *aboutAction;

    SettingsDialog *settingsDialog;

    QUdpSocket *udpSocket;
    QStringList sendAddress;
    uint port;
    QString username;
    QString key;
    QNetworkAccessManager *nmg;
    QNetworkReply *textReply, *htmlReply, *imageReply;
    QMimeData recivedMimeData;
};

#endif // CLIPBOARDMANAGER_H
