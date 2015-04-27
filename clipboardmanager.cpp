#include "clipboardmanager.h"
#include "settingsdialog.h"
#include "qencryptrc4.h"
#include <QApplication>
#include <QtWidgets>
#include <QtNetwork/QtNetwork>
#include <qhttpserver.h>
#include <qhttprequest.h>
#include <qhttpresponse.h>

ClipboardManager::ClipboardManager(QObject *parent) : QObject(parent)
{
    udpSocket = NULL;
    nmg = new QNetworkAccessManager;
    QSettings settings;
    if (settings.value("username", "").toString().length() == 0)
        showSettingsDialog();
    reload();
}

ClipboardManager::~ClipboardManager()
{

}

void ClipboardManager::createTrayIcon()
{
    createTrayMenu();
    trayIcon = new QSystemTrayIcon(QIcon(":/icons/systray.png"), NULL);
    trayIcon->setContextMenu(trayIconMenu);
    connect(trayIcon, &QSystemTrayIcon::activated, this, &ClipboardManager::iconActivated);
    trayIcon->show();
}

void ClipboardManager::createTrayMenu()
{
    sendClipAction = new QAction(tr("Send"), this);
    connect(sendClipAction, &QAction::triggered, this, &ClipboardManager::sendClipboard);

    settingsAction = new QAction(tr("Settings..."), this);
    connect(settingsAction, &QAction::triggered, this, &ClipboardManager::showSettingsDialog);

    quitAction = new QAction(tr("Quit"), this);
    connect(quitAction, &QAction::triggered, this, &ClipboardManager::quit);

    trayIconMenu = new QMenu();
    trayIconMenu->addAction(sendClipAction);
    trayIconMenu->addAction(settingsAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

}

void ClipboardManager::sendClipboard()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << username << QHostInfo::localHostName();
    QStringList typeList;
    if (clipboard->text().length())
        typeList.append("text");
    if (!clipboard->image().isNull())
        typeList.append("image");
    QString typeText = typeList.join(',');
    out << typeText;

    foreach (QString address, sendAddress) {
        udpSocket->writeDatagram(data, QHostAddress(address), port);
    }
}

void ClipboardManager::reciveData()
{
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        QHostAddress peerAddress;
        quint16 peerPort;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(), datagram.size(), &peerAddress, &peerPort);
        QDataStream in(&datagram, QIODevice::ReadOnly);
        QString toUsername, hostname;
        QString typeText;
        in >> toUsername >> hostname >> typeText;
        if (toUsername != username)
            continue;
        if (hostname == QHostInfo::localHostName())
            continue;

        foreach (QString type, typeText.split(',')) {
            if (type == "text") {
                QString url = QString("http://%1:%2/clipboard/text").arg(peerAddress.toString()).arg(port);
                textReply = nmg->get(QNetworkRequest(url));
                connect(textReply, &QNetworkReply::finished, this, &ClipboardManager::getTextFinish);
            }
            else if (type == "image") {
                QString url = QString("http://%1:%2/clipboard/image").arg(peerAddress.toString()).arg(port);
                imageReply = nmg->get(QNetworkRequest(url));
                connect(imageReply, &QNetworkReply::finished, this, &ClipboardManager::getImageFinish);
            }
        }
    }
}

void ClipboardManager::handleHttp(QHttpRequest *req, QHttpResponse *resp)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QEncryptRc4 rc4;
    rc4.UseKey(key);
    qDebug() << req->path();
    if (req->path() == "/clipboard/text") {
        resp->writeHead(200);
        resp->setHeader("Content-Type", "text/encrypted-plain");

        QByteArray text = clipboard->text().toUtf8();
        QByteArray data;
        rc4.Encrypt(text, data);
        resp->write(data);
    }
    else if (req->path() == "/clipboard/image") {
        resp->writeHead(200);
        resp->setHeader("Content-Type", "image/encrypted-png");

        QImage image = clipboard->image();
        qDebug() << "send image: " << image.width() << image.height();

        QByteArray imageData;
        QBuffer buf(&imageData);
        buf.open(QIODevice::WriteOnly);
        image.save(&buf, "PNG");
        buf.close();
        qDebug() << "imagedata size:" << imageData.size();
        QByteArray data;
        rc4.Encrypt(imageData, data);
//        data = imageData;
        resp->write(data);
    }
    else {
        resp->writeHead(404);
    }
    resp->end();
}

void ClipboardManager::getTextFinish()
{
    if (!textReply->error()) {
        QClipboard *clipboard = QGuiApplication::clipboard();
        QByteArray data = textReply->readAll();
        QByteArray text;
        QEncryptRc4 rc4;
        rc4.UseKey(key);
        rc4.Encrypt(data, text);
        clipboard->setText(QString::fromUtf8(text));
    } else {
        qDebug() << textReply->errorString();
    }
    textReply->deleteLater();
}

void ClipboardManager::getImageFinish()
{
    qDebug() << "recived image clipboard";
    if (!imageReply->error()) {
        QClipboard *clipboard = QGuiApplication::clipboard();
        QByteArray data = imageReply->readAll();
        QByteArray imageData;
        QEncryptRc4 rc4;
        rc4.UseKey(key);
        rc4.Encrypt(data, imageData);
//        imageData = data;
        QImage image = QImage::fromData(imageData);
        qDebug() << "recived image: " << image.width() << image.height();
        clipboard->setImage(image);
    } else {
        qDebug() << textReply->errorString();
    }
    imageReply->deleteLater();
}

void ClipboardManager::showSettingsDialog()
{
    SettingsDialog dlg;
    if(dlg.exec() == QDialog::Accepted) {
        reload();
    }
}

void ClipboardManager::reload()
{
    if (udpSocket) {
        udpSocket->close();
        delete udpSocket;
    }
    QSettings settings;
    username = settings.value("username", "someone").toString();
    key = settings.value("key", "l2kf322ldXD").toString();
    port = settings.value("port", 8831).toUInt();
    sendAddress = settings.value("sendtoips", QStringList("255.255.255.255")).toStringList();

    // Listen to udp port for notifications
    udpSocket = new QUdpSocket(this);
    udpSocket->bind(port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(udpSocket, &QUdpSocket::readyRead, this, &ClipboardManager::reciveData);

    // Listen to tcp port for http requests
    QHttpServer *server = new QHttpServer;
    connect(server, SIGNAL(newRequest(QHttpRequest*, QHttpResponse*)),
            this, SLOT(handleHttp(QHttpRequest*, QHttpResponse*)));

    server->listen(port);
}

void ClipboardManager::quit()
{
    QApplication::quit();
}

void ClipboardManager::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
        sendClipboard();
        break;
    default:
        break;
    }
}
