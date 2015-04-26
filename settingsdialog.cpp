#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QtGui>
#include <QtWidgets>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::showEvent(QShowEvent * event)
{
    QSettings settings;
    ui->usernameEdit->setText(settings.value("username", "someone").toString());
    ui->keyEdit->setText(settings.value("key", "l2kf322ldXD").toString());
    ui->portEdit->setText(settings.value("port", 8831).toString());
    QStringList sendtoips = settings.value("sendtoips", QStringList("255.255.255.255")).toStringList();
    ui->ipEdit->setPlainText(sendtoips.join('\n'));
}

void SettingsDialog::on_cancelButton_clicked()
{
    reject();
}

void SettingsDialog::on_saveButton_clicked()
{
    QSettings settings;
    settings.setValue("username", ui->usernameEdit->text());
    settings.setValue("key", ui->keyEdit->text());
    settings.setValue("port", ui->portEdit->text().toUInt());
    settings.setValue("sendtoips", ui->ipEdit->toPlainText().split('\n', QString::SkipEmptyParts));
    accept();
}
