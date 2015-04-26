#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QSystemTrayIcon;
class QMenu;

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

private slots:
    void on_cancelButton_clicked();
    void on_saveButton_clicked();

protected:
    void showEvent(QShowEvent * event);

private:
    Ui::SettingsDialog *ui;
};

#endif // SETTINGSDIALOG_H
