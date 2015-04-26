#include "clipboardmanager.h"
#include "settingsdialog.h"
#include "qencryptrc4.h"
#include <QApplication>
#include <QtWidgets>
#include <QtNetwork/QtNetwork>
#include <iostream>

ClipboardManager::ClipboardManager(QObject *parent) : QObject(parent)
{
    udpSocket = NULL;
    settingsDialog = new SettingsDialog();
    connect(settingsDialog, &SettingsDialog::accepted, this, &ClipboardManager::reload);
    QSettings settings;
    if (settings.value("username", "").toString().length() == 0)
        settingsDialog->show();
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

    QByteArray originText = clipboard->text().toUtf8();
    QEncryptRc4 rc4;
    rc4.UseKey(key);
    QByteArray text;
    rc4.Encrypt(originText, text);

    foreach (QString address, sendAddress) {
        int length = text.length();
        int start = 0;
        int chunk_size = 480;
        short int i = 0;
        while (start < length) {
            short int last = 0;
            if (start + chunk_size >= length) {
                last = 1;
            }
            QByteArray data;
            QDataStream out(&data, QIODevice::WriteOnly);
            out << username << QHostInfo::localHostName() << i << last << text.mid(start, chunk_size);
            udpSocket->writeDatagram(data, QHostAddress(address), port);
            start += chunk_size;
            i++;
        }
    }
}

void ClipboardManager::reciveData()
{
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(), datagram.size());
        QDataStream in(&datagram, QIODevice::ReadOnly);
        QString toUsername, hostname;
        QByteArray text;
        short int i;
        short int last;
        in >> toUsername >> hostname >> i >> last >> text;
        if (toUsername != username)
            continue;
        if (hostname == QHostInfo::localHostName())
            continue;

        if (i == 0) {
            buffer.clear();
        }
        buffer.append(text);

        if (last) {
            QByteArray originText;
            QEncryptRc4 rc4;
            rc4.UseKey(key);
            rc4.Encrypt(buffer, originText);

            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setText(QString::fromUtf8(originText));
        }
    }
}

void ClipboardManager::showSettingsDialog()
{
    settingsDialog->show();
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
    udpSocket = new QUdpSocket(this);
    udpSocket->bind(port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(udpSocket, &QUdpSocket::readyRead, this, &ClipboardManager::reciveData);
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
