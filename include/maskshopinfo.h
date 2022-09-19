#ifndef MASKSHOPINFO_H
#define MASKSHOPINFO_H

#include <QWidget>

namespace Ui {
class MaskshopInfo;
}

class MaskshopInfo : public QWidget
{
    Q_OBJECT

public:
    explicit MaskshopInfo(const QString &name = QString(), const QString &gpg = QString(),
                          const QString &host = QString(), int port = 0,  const QString &user = QString(), const QString &pass = QString(),
                          const QString &archiveDir = QString(), const QString &olDir = QString(), const QString &cdDir = QString(), const QString &regDir = QString(),
                          const QString &recipients = QString(), const QString &subject = QString(), const QString &letter = QString(),
                          bool def = false,
                          QWidget *parent = 0);
    ~MaskshopInfo();

    void setReadonly(bool);
    void save();

    QString name();
    QString gpg();
    QString host();
    int port();
    QString username();
    QString password();
    QString archiveDir();
    QString olDir();
    QString cdDir();
    QString regDir();
    QString recipients();
    QString subject();
    QString letter();

    void setName(const QString &name);
    void setGpg(const QString &smthng);
    void setHost(const QString &host);
    void setPort(int port);
    void setUsername(const QString &user);
    void setPassword(const QString &pass);
    void setArchiveDir(const QString &archiveDir);
    void setOlDir(const QString &olDir);
    void setCdDir(const QString &cdDir);
    void setRegDir(const QString &regDir);
    void setRecipients(const QString &recipients);
    void setSubject(const QString &subject);
    void setLetter(const QString &letter);

    bool _default;

private:
    QString _name;
    QString _gpg;
    QString _host;
    int _port;
    QString _user;
    QString _pass;
    QString _archiveDir;
    QString _olDir;
    QString _cdDir;
    QString _regDir;
    QString _recipients;
    QString _subject;
    QString _letter;
    Ui::MaskshopInfo *ui;

private slots:
    void onImportPbClicked();
    void on_ftpGb_toggled(bool arg1);
    void on_letterGb_toggled(bool arg1);
    void on_infoPb_clicked();
};

#endif // MASKSHOPINFO_H
