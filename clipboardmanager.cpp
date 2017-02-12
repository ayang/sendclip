#include "qhttpserver.hpp"
#include "qhttpserverconnection.hpp"
#include "qhttpserverrequest.hpp"
#include "qhttpserverresponse.hpp"

#include "clipboardmanager.h"
#include "settingsdialog.h"
#include "aboutdialog.h"
#include "qencryptrc4.h"
#include <QApplication>
#include <QtWidgets>
#include <QtNetwork/QtNetwork>

using namespace qhttp::server;

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
    if (udpSocket) {
        udpSocket->close();
        delete udpSocket;
    }
    delete nmg;
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

    aboutAction = new QAction(tr("About"), this);
    connect(aboutAction, &QAction::triggered, this, &ClipboardManager::about);

    trayIconMenu = new QMenu();
    trayIconMenu->addAction(sendClipAction);
    trayIconMenu->addAction(settingsAction);
    trayIconMenu->addAction(aboutAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

}

void ClipboardManager::sendClipboard()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QString data;
    data.append(username);
    data.append("\n");
    data.append(QHostInfo::localHostName());
    data.append("\n");
    QStringList typeList;
    if (clipboard->text().length())
        typeList.append("text");
    if (!clipboard->image().isNull())
        typeList.append("image");
    if (!clipboard->mimeData()->html().length())
        typeList.append("html");
    QString typeText = typeList.join(',');
    data.append(typeText);

    foreach (QString address, sendAddress) {
        qDebug() << data;
        udpSocket->writeDatagram(data.toLocal8Bit(), QHostAddress(address), port);
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
        QString data(datagram);
        QStringList lines = data.split("\n");
        if (lines.count() < 3)
            continue;
        QString toUsername, hostname;
        QString typeText;
        toUsername = lines[0];
        hostname = lines[1];
        typeText = lines[2];
        if (toUsername != username)
            continue;
        if (hostname == QHostInfo::localHostName())
            continue;

        recivedMimeData.clear();
        trayIcon->showMessage(tr("Clipboard Recived"), tr("%1(%2) send a clipboard with %3")
                              .arg(hostname, peerAddress.toString(), typeText));

        foreach (QString type, typeText.split(',')) {
            if (type == "text") {
                QString url = QString("http://%1:%2/clipboard/text").arg(peerAddress.toString()).arg(port);
                textReply = nmg->get(QNetworkRequest(url));
                connect(textReply, &QNetworkReply::finished, this, &ClipboardManager::getTextFinish);
            }
            else if (type == "html") {
                QString url = QString("http://%1:%2/clipboard/html").arg(peerAddress.toString()).arg(port);
                htmlReply = nmg->get(QNetworkRequest(url));
                connect(htmlReply, &QNetworkReply::finished, this, &ClipboardManager::getHtmlFinish);
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
    qDebug() << req->url().path();
    if (req->url().path() == "/clipboard/text") {
        resp->setStatusCode(qhttp::ESTATUS_OK);
        resp->addHeader("Content-Type", "text/encrypted-plain");

        QByteArray data = clipboard->text().toUtf8();
        resp->write(encrypt(data));
    }
    else if (req->url().path() == "/clipboard/html") {
        resp->setStatusCode(qhttp::ESTATUS_OK);
        resp->addHeader("Content-Type", "text/encrypted-html");

        QByteArray data = clipboard->mimeData()->html().toUtf8();
        resp->write(encrypt(data));
    }
    else if (req->url().path() == "/clipboard/image") {
        resp->setStatusCode(qhttp::ESTATUS_OK);
        resp->addHeader("Content-Type", "image/encrypted-png");

        QImage image = clipboard->image();
        qDebug() << "send image: " << image.width() << image.height();

        QByteArray imageData;
        QBuffer buf(&imageData);
        buf.open(QIODevice::WriteOnly);
        image.save(&buf, "PNG");
        buf.close();
        qDebug() << "imagedata size:" << imageData.size();

        resp->write(encrypt(imageData));
    }
    else {
        resp->setStatusCode(qhttp::ESTATUS_NOT_FOUND);
    }
    resp->end();
}

void ClipboardManager::getTextFinish()
{
    qDebug() << "recived text clipboard";
    if (!textReply->error()) {
        QClipboard *clipboard = QGuiApplication::clipboard();
        QByteArray data = textReply->readAll();
        QString text = QString::fromUtf8(decrypt(data));
        recivedMimeData.setText(text);
        clipboard->setMimeData(&recivedMimeData);
    } else {
        qDebug() << "Receive text error: " << textReply->errorString();
    }
    textReply->deleteLater();
}

void ClipboardManager::getHtmlFinish()
{
    qDebug() << "recived text clipboard";
    if (!htmlReply->error()) {
        QClipboard *clipboard = QGuiApplication::clipboard();
        QByteArray data = htmlReply->readAll();
        QString html = QString::fromUtf8(decrypt(data));
        recivedMimeData.setHtml(html);
        clipboard->setMimeData(&recivedMimeData);
    } else {
        qDebug() << "Receive html error: " << htmlReply->errorString();
    }
    htmlReply->deleteLater();
}

void ClipboardManager::getImageFinish()
{
    qDebug() << "recived image clipboard";
    if (!imageReply->error()) {
        QClipboard *clipboard = QGuiApplication::clipboard();
        QByteArray data = imageReply->readAll();
        QImage image = QImage::fromData(decrypt(data));
        qDebug() << "recived image: " << image.width() << image.height();
        recivedMimeData.setImageData(image);
        clipboard->setMimeData(&recivedMimeData);
    } else {
        qDebug() << "Receive image error: " << imageReply->errorString();
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
    udpSocket->bind(QHostAddress::AnyIPv4, port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
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

void ClipboardManager::about()
{
    AboutDialog dlg;
    dlg.exec();
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

QByteArray ClipboardManager::encrypt(const QByteArray &data)
{
    QByteArray encryptedData;
    QEncryptRc4 rc4;
    rc4.UseKey(key);
    rc4.Encrypt(data, encryptedData);
    return encryptedData;
}

QByteArray ClipboardManager::decrypt(const QByteArray &data)
{
    return encrypt(data);
}
