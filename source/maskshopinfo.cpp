#include "include/maskshopinfo.h"
#include "ui_maskshopinfo.h"
#include <QProcess>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

MaskshopInfo::MaskshopInfo(const QString &name, const QString &gpg,
                           const QString &host, int port, const QString &user, const QString &pass,
                           const QString &archiveDir, const QString &olDir, const QString &cdDir, const QString &regDir,
                           const QString &recipients, const QString &subject, const QString &letter,
                           bool def,
                           QWidget *parent) :
    QWidget(parent),
    _default(def),
    _name(name), _gpg(gpg),
    _host(host), _port(port), _user(user), _pass(pass),
    _archiveDir(archiveDir), _olDir(olDir), _cdDir(cdDir), _regDir(regDir),
    _recipients(recipients), _subject(subject), _letter(letter),
    ui(new Ui::MaskshopInfo)
{
    ui->setupUi(this);

    ui->nameLe->setText(_name);
    ui->gpgLe->setText(_gpg);
    ui->hostLe->setText(_host);
    ui->portLe->setText(QString::number(_port));
    ui->userLe->setText(_user);
    ui->passLe->setText(_pass);
    ui->archiveDirLe->setText(_archiveDir);
    ui->olDirLe->setText(_olDir);
    ui->cdDirLe->setText(_cdDir);
    ui->regDirLe->setText(_regDir);
    ui->recipientsLe->setText(_recipients);
    ui->subjectLe->setText(_subject);
    ui->letterTe->setText(_letter);

    if (!_gpg.isEmpty()) {
        ui->importPb->setText("Изменить");
        QProcess p;
        p.start("gpg --list-keys " + _gpg);
        p.waitForFinished(-1);
        ui->expireLb->setText("<font color='red'>"+QString(p.readAllStandardOutput()).section(QRegExp("[\\[\\]]"),1,1)+"</font>");
    }
    connect(ui->importPb, SIGNAL(clicked(bool)), this, SLOT(onImportPbClicked()));
}

MaskshopInfo::~MaskshopInfo()
{
    delete ui;
}

void MaskshopInfo::setReadonly(bool enable)
{
    ui->nameLe->setReadOnly(enable);
    ui->gpgLe->setReadOnly(enable);
    ui->hostLe->setReadOnly(enable);
    ui->portLe->setReadOnly(enable);
    ui->userLe->setReadOnly(enable);
    ui->passLe->setReadOnly(enable);
    ui->archiveDirLe->setReadOnly(enable);
    ui->olDirLe->setReadOnly(enable);
    ui->cdDirLe->setReadOnly(enable);
    ui->regDirLe->setReadOnly(enable);
    ui->recipientsLe->setReadOnly(enable);
    ui->subjectLe->setReadOnly(enable);
    ui->letterTe->setReadOnly(enable);
    ui->importPb->setEnabled(!enable);
    ui->ftpGb->setCheckable(!enable);
    ui->letterGb->setCheckable(!enable);
}

void MaskshopInfo::save()
{
    setName(ui->nameLe->text());
    setGpg(ui->gpgLe->text());
    setHost(ui->hostLe->text());
    setPort(ui->portLe->text().toInt());
    setUsername(ui->userLe->text());
    setPassword(ui->passLe->text());
    setArchiveDir(ui->archiveDirLe->text());
    setOlDir(ui->olDirLe->text());
    setCdDir(ui->cdDirLe->text());
    setRegDir(ui->regDirLe->text());
    setRecipients(ui->recipientsLe->text());
    setSubject(ui->subjectLe->text());
    setLetter(ui->letterTe->toPlainText());
}

QString MaskshopInfo::name()
{
    return _name;
}

QString MaskshopInfo::gpg()
{
    return _gpg;
}

QString MaskshopInfo::host()
{
    return _host;
}

int MaskshopInfo::port()
{
    return _port;
}

QString MaskshopInfo::username()
{
    return _user;
}

QString MaskshopInfo::password()
{
    return _pass;
}

QString MaskshopInfo::archiveDir()
{
    return _archiveDir;
}

QString MaskshopInfo::olDir()
{
    return _olDir;
}

QString MaskshopInfo::cdDir()
{
    return _cdDir;
}

QString MaskshopInfo::regDir()
{
    return _regDir;
}

QString MaskshopInfo::recipients()
{
    return _recipients;
}

QString MaskshopInfo::subject()
{
    return _subject;
}

QString MaskshopInfo::letter()
{
    return _letter;
}

void MaskshopInfo::onImportPbClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Укажите файл ключа", QDir::homePath(), "*.asc *.key *.txt");
    if (!fileName.isEmpty())
    {
        QProcess p;
        p.start("gpg --import " + fileName);
        p.waitForFinished(-1);

        QString output = p.readAllStandardError();

        setGpg(output.mid(output.indexOf(QRegExp("[A-F0-9]{8}")), 8));
    }
}

void MaskshopInfo::setName(const QString &name)
{
    if (!name.isEmpty())
    {
        _name = name;
        ui->nameLe->setText(_name);
    }
}

void MaskshopInfo::setGpg(const QString &code)
{
    _gpg = code;
    ui->gpgLe->setText(_gpg);
    ui->importPb->setText("Изменить");
}

void MaskshopInfo::setHost(const QString &host)
{
    _host = host;
    ui->hostLe->setText(_host);
}

void MaskshopInfo::setPort(int port)
{
    _port = port;
    ui->portLe->setText(QString::number(_port));
}

void MaskshopInfo::setUsername(const QString &user)
{
    _user = user;
    ui->userLe->setText(_user);
}

void MaskshopInfo::setPassword(const QString &pass)
{
    _pass = pass;
    ui->passLe->setText(_pass);
}

void MaskshopInfo::setArchiveDir(const QString &archiveDir)
{
    _archiveDir = archiveDir;
    ui->archiveDirLe->setText(_archiveDir);
}

void MaskshopInfo::setOlDir(const QString &olDir)
{
    _olDir = olDir;
    ui->olDirLe->setText(_olDir);
}

void MaskshopInfo::setCdDir(const QString &cdDir)
{
    _cdDir = cdDir;
    ui->cdDirLe->setText(_cdDir);
}

void MaskshopInfo::setRegDir(const QString &regDir)
{
    _regDir = regDir;
    ui->regDirLe->setText(_regDir);
}

void MaskshopInfo::setRecipients(const QString &recipients)
{
    _recipients = recipients;
    ui->recipientsLe->setText(_recipients);
}

void MaskshopInfo::setSubject(const QString &subject)
{
    _subject = subject;
    ui->subjectLe->setText(_subject);
}

void MaskshopInfo::setLetter(const QString &letter)
{
    _letter = letter;
    ui->letterTe->setText(_letter);
}

void MaskshopInfo::on_ftpGb_toggled(bool arg1)
{
    ui->letterGb->setChecked(arg1);
    if (arg1)
    {
        ui->hostLe->setText(_host);
        ui->portLe->setText(QString::number(_port));
        ui->userLe->setText(_user);
        ui->passLe->setText(_pass);
        ui->archiveDirLe->setText(_archiveDir);
        ui->olDirLe->setText(_olDir);
        ui->cdDirLe->setText(_cdDir);
        ui->regDirLe->setText(_regDir);
    }
    else
    {
        ui->hostLe->setText("");
        ui->portLe->setText("");
        ui->userLe->setText("");
        ui->passLe->setText("");
        ui->archiveDirLe->setText("");
        ui->olDirLe->setText("");
        ui->cdDirLe->setText("");
        ui->regDirLe->setText("");
    }
}

void MaskshopInfo::on_letterGb_toggled(bool arg1)
{
    ui->letterTe->setEnabled(arg1);
    if (arg1)
    {
        ui->recipientsLe->setText(_recipients);
        ui->subjectLe->setText(_subject);
        ui->letterTe->setText( _letter);
    }
    else
    {
        ui->recipientsLe->setText("");
        ui->subjectLe->setText("");
        ui->letterTe->setText("");
    }
}

void MaskshopInfo::on_infoPb_clicked()
{
    QCoreApplication* app = QApplication::instance();
    QString title = app->applicationName().append(" ").append(app->applicationVersion().append(" - info"));
    QString text = "Плейсхолдер должен быть заключён в спецсимвол % с обеих сторон\n\
Доступные плейсхолдеры:\n\
%MPW%, %STARTTIME%, %d%, %dd%, %MM%, %yy%, %yyyy%, %h%, %hh%, %m%, %mm%, %s%, %ss%";
    QMessageBox::information(this, title, text);
}
