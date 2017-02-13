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

class ReceivedData
{
public:

    ReceivedData(): version(0), types(), receivedTypes(), text(), html(), image() {}

    void clear();

    int newVersion();

    int newVersion(const QStringList &types);

    void setText(const QString &text);
    void setHtml(const QString &html);
    void setImage(const QImage &image);

    bool completed() const;

    void setClipboard();

    int getVersion() const;

    QStringList getTypes() const;

    QString getText() const;

    QString getHtml() const;

    QImage getImage() const;

private:
    int version;
    QStringList types;
    QSet<QString> receivedTypes;
    QString text;
    QString html;
    QImage image;
};

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
    QMutex dataMutex;
    ReceivedData received;
};

#endif // CLIPBOARDMANAGER_H
