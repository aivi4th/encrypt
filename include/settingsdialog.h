#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QList>
#include "maskshopinfo.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT
    
public:
    enum Status {Synchronised, Unsynchronized};

    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

public slots:
    void accept();
    void reject();

signals:
    void statusChanged(Status status);
    
private:
    Status _status;
    Ui::SettingsDialog *ui;

private slots:
    void addRecipient(const QString &name = QString(), const QString &code = QString(),
                      const QString &host = QString(), int port = 0,  const QString &user = QString(), const QString &pass = QString(),
                      const QString &archiveDir = QString(), const QString &olDir = QString(), const QString &cdDir = QString(), const QString &regDir = QString(),
                      const QString &recipients = QString(), const QString &subject = QString(), const QString &letter = QString(),
                      bool def = false);
    void clearLayout(QLayout* layout);
    void loadSettings();
    void onPlusButtonClicked(bool);
    void onStatusChanged(Status status);
    void onTabCloseRequested(int index);
    void saveSettings();
    void setDefaults();
    void setStatus(Status status);
    void showTooltip();
    Status status();
    void updateStatus();
    void on__selfCodePb_clicked();
    void on__keyOpfsh_toggled(bool checked);
};

#endif // SETTINGSDIALOG_H
