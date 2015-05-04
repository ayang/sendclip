#include "clipboardmanager.h"
#include "settingsdialog.h"
#include "aboutdialog.h"
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
    connect(nmg, &QNetworkAccessManager::finished, this, &ClipboardManager::dataReplyFinish);
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
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << username << QHostInfo::localHostName();
    QStringList typeList;
    qDebug() << "Send clipboard formats:" << clipboard->mimeData()->formats();

    foreach (QString format, clipboard->mimeData()->formats()) {
        if (format=="text/plain"||format=="text/html"||format=="text/richtext"||
                format=="image/png"||format=="image/tiff"||format=="application/x-qt-image") {
            typeList.append(format);
        }
    }
    if (typeList.empty())
        return;
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
        QString formatString;
        qDebug() << formatString;
        in >> toUsername >> hostname >> formatString;
        if (toUsername != username)
            continue;
        if (hostname == QHostInfo::localHostName())
            continue;

        curFormats.clear();
        mimeDatas.clear();

        foreach (QString format, formatString.split(',')) {
            QString urlString = QString("http://%1:%2/clipboard").arg(peerAddress.toString()).arg(port);
            QNetworkRequest request(urlString);
            request.setRawHeader("Clipboard-Format", format.toLatin1());
            QNetworkReply *reply = nmg->get(request);
            curFormats.append(format);
        }
    }
}

void ClipboardManager::handleHttp(QHttpRequest *req, QHttpResponse *resp)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QEncryptRc4 rc4;
    rc4.UseKey(key);
    if (req->path() == "/clipboard") {
        QString format = req->header("Clipboard-Format");
        resp->setHeader("Clipboard-Format", format);
        resp->writeHead(200);

        QByteArray data = clipboard->mimeData()->data(format);
        QByteArray rawData;
        rc4.Encrypt(data, rawData);
        resp->write(rawData);
    }
    else {
        resp->writeHead(404);
    }
    resp->end();
}

void ClipboardManager::dataReplyFinish(QNetworkReply *reply)
{
    if (!reply->error()) {
        QClipboard *clipboard = QGuiApplication::clipboard();
        QByteArray rawData = reply->readAll();
        QByteArray data;
        QEncryptRc4 rc4;
        rc4.UseKey(key);
        rc4.Encrypt(rawData, data);
        QString format = QString::fromLatin1(reply->rawHeader("Clipboard-Format"));
        mimeDatas[format] = data;
        if (mimeDatas.size() == curFormats.size()) {
            qDebug() << curFormats;
            QMimeData *mime = new QMimeData;
            foreach (QString format, curFormats) {
                mime->setData(format, mimeDatas[format]);
            }
            clipboard->setMimeData(mime);
        }
    } else {
        qDebug() << reply->errorString();
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
