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

int ReceivedData::getVersion() const
{
    return version;
}

QStringList ReceivedData::getTypes() const
{
    return types;
}

QString ReceivedData::getText() const
{
    return text;
}

QString ReceivedData::getHtml() const
{
    return html;
}

QImage ReceivedData::getImage() const
{
    return image;
}

void ReceivedData::clear() {
    types.clear();
    receivedTypes.clear();
    text.clear();
    html.clear();
    image = QImage();
}

int ReceivedData::newVersion() {
    version++;
    clear();
    return version;
}


int ReceivedData::newVersion(const QStringList &types) {
    version++;
    clear();
    this->types = types;
    return version;
}

void ReceivedData::setText(const QString &text)
{
    this->text = text;
    receivedTypes.insert("text");
}

void ReceivedData::setHtml(const QString &html)
{
    this->html = html;
    receivedTypes.insert("html");
}

void ReceivedData::setImage(const QImage &image)
{
    this->image = image;
    receivedTypes.insert("image");
}

bool ReceivedData::completed() const {
    foreach (QString type, types) {
        if (!receivedTypes.contains(type)) {
            return false;
        }
    }
    return true;
}

void ReceivedData::setClipboard() {
    QClipboard *clipboard = QGuiApplication::clipboard();
    QMimeData *mime = new QMimeData;
    if (text.length() > 0) {
        mime->setText(text);
    }
    if (html.length() > 0) {
        mime->setHtml(html);
    }
    if (!image.isNull()) {
        mime->setImageData(image);
    }
    clipboard->setMimeData(mime);
}

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
    if (clipboard->text().length() > 0)
        typeList.append("text");
    if (!clipboard->image().isNull())
        typeList.append("image");
    if (clipboard->mimeData()->html().length() > 0)
        typeList.append("html");
    QString typeText = typeList.join(',');
    data.append(typeText);

    qDebug() << "udp send: " << data;
    foreach (QString address, sendAddress) {
        udpSocket->writeDatagram(data.toLocal8Bit(), QHostAddress(address), port);
    }
}

QNetworkReply * ClipboardManager::requestClipboard(QString url)
{
    qDebug()<<"get: "<<url;
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    QNetworkReply *reply = nmg->get(request);
    reply->setProperty("version", received.getVersion());

    return reply;
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
        qDebug() << "udp received: " << data;
        QStringList lines = data.split("\n");
        if (lines.count() < 3)
            continue;
        QString toUsername, hostname;
        QString typeText;
        toUsername = lines[0];
        hostname = lines[1];
        typeText = lines[2];
        QStringList types = typeText.split(',');
        if (toUsername != username)
            continue;
        if (hostname == QHostInfo::localHostName())
            continue;

        dataMutex.lock();
        int version = received.newVersion(types);
        dataMutex.unlock();
        trayIcon->showMessage(tr("Clipboard Recived"), tr("%1(%2) send a clipboard with %3")
                              .arg(hostname, peerAddress.toString(), typeText), QSystemTrayIcon::Information, 3000);

        foreach (QString type, types) {
            if (type == "text") {
                QString url = QString("http://%1:%2/clipboard/text").arg(peerAddress.toString()).arg(port);
                QNetworkReply *reply = requestClipboard(url);
                connect(reply, &QNetworkReply::finished, this, &ClipboardManager::getTextFinish);
            }
            else if (type == "html") {
                QString url = QString("http://%1:%2/clipboard/html").arg(peerAddress.toString()).arg(port);
                QNetworkReply *reply = requestClipboard(url);
                connect(reply, &QNetworkReply::finished, this, &ClipboardManager::getHtmlFinish);
            }
            else if (type == "image") {
                QString url = QString("http://%1:%2/clipboard/image").arg(peerAddress.toString()).arg(port);
                QNetworkReply *reply = requestClipboard(url);
                connect(reply, &QNetworkReply::finished, this, &ClipboardManager::getImageFinish);
            }
        }
    }
}

void ClipboardManager::handleHttp(QHttpRequest *req, QHttpResponse *resp)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QEncryptRc4 rc4;
    rc4.UseKey(key);
    qDebug() << "serve: " << req->url().path();
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
    QNetworkReply *reply = dynamic_cast<QNetworkReply*>(sender());
    if (!reply->error()) {
        QByteArray data = reply->readAll();
        QString text = QString::fromUtf8(decrypt(data));
        int version = reply->property("version").toInt();
        dataMutex.lock();
        if (version == received.getVersion()) {
            received.setText(text);
            received.setClipboard();
        }
        dataMutex.unlock();
    } else {
        qDebug() << "Receive text error: " << reply->errorString();
    }
    reply->deleteLater();
}

void ClipboardManager::getHtmlFinish()
{
    qDebug() << "recived html clipboard";
    QNetworkReply *reply = dynamic_cast<QNetworkReply*>(sender());
    if (!reply->error()) {
        QByteArray data = reply->readAll();
        QString html = QString::fromUtf8(decrypt(data));
        int version = reply->property("version").toInt();
        dataMutex.lock();
        if (version == received.getVersion()) {
            received.setHtml(html);
            received.setClipboard();
        }
        dataMutex.unlock();
    } else {
        qDebug() << "Receive html error: " << reply->errorString();
    }
    reply->deleteLater();
}

void ClipboardManager::getImageFinish()
{
    qDebug() << "recived image clipboard";
    QNetworkReply *reply = dynamic_cast<QNetworkReply*>(sender());
    if (!reply->error()) {
        QByteArray data = reply->readAll();
        QImage image = QImage::fromData(decrypt(data));
        qDebug() << "recived image: " << image.width() << image.height();
        int version = reply->property("version").toInt();
        dataMutex.lock();
        if (version == received.getVersion()) {
            received.setImage(image);
            received.setClipboard();
        }
        dataMutex.unlock();
    } else {
        qDebug() << "Receive image error: " << reply->errorString();
    }
    reply->deleteLater();
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
