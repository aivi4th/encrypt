#include <QCommonStyle>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QTextEdit>
#include <QProcess>
#include <QFileDialog>
#include "include/settingsdialog.h"
#include "ui_settingsdialog.h"

// Constructor 1
SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    ui->_settingsTw->setStyleSheet("QTabBar::tab { height: 25px; width: 150px; }");

    QPushButton * plusButton = new QPushButton(QIcon(":/icons/insert"), "");
    plusButton->setIconSize(QSize(16,16));
    ui->tabWidget->setCornerWidget(plusButton, Qt::TopLeftCorner);

    ui->_defaultPb->setIcon(QCommonStyle().standardIcon(QStyle::SP_DialogResetButton));
    ui->_defaultPb->setIconSize(QSize(16,16));
    ui->_okPb->setIcon(QCommonStyle().standardIcon(QStyle::SP_DialogOkButton));
    ui->_okPb->setIconSize(QSize(16,16));
    ui->_closePb->setIcon(QCommonStyle().standardIcon(QStyle::SP_DialogCancelButton));
    ui->_closePb->setIconSize(QSize(16,16));

    connect(this, SIGNAL(statusChanged(Status)), this, SLOT(onStatusChanged(Status)));
    connect(ui->_defaultPb, SIGNAL(clicked()), this, SLOT(setDefaults()));
    connect(ui->_fsRootPb, SIGNAL(clicked()), this, SLOT(showTooltip()));
    connect(ui->_svnRootPb, SIGNAL(clicked()), this, SLOT(showTooltip()));
    connect(ui->_svnIgnorePb, SIGNAL(clicked()), this, SLOT(showTooltip()));
    connect(ui->_fsRootLe, SIGNAL(textChanged(QString)), this, SLOT(updateStatus()));
    connect(ui->_svnRootLe, SIGNAL(textChanged(QString)), this, SLOT(updateStatus()));
    connect(ui->_svnIgnoreLe, SIGNAL(textChanged(QString)), this, SLOT(updateStatus()));
    connect(ui->_selfCodeLe, SIGNAL(textChanged(QString)), this, SLOT(updateStatus()));
    connect(ui->_selfCodeLe, SIGNAL(textChanged(QString)), this, SLOT(updateStatus()));
    connect(ui->_mailUserLe, SIGNAL(textChanged(QString)), this, SLOT(updateStatus()));
    connect(ui->_mailPassLe, SIGNAL(textChanged(QString)), this, SLOT(updateStatus()));
    connect(plusButton, SIGNAL(clicked(bool)), this, SLOT(onPlusButtonClicked(bool)));
    connect(ui->tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(onTabCloseRequested(int)));

    loadSettings();
    ui->_settingsTw->setCurrentIndex(0);
}



// Destructor
SettingsDialog::~SettingsDialog()
{
    delete ui;
}



// Reimplementation of QDialog accept() slot
void SettingsDialog::accept()
{
    if (_status == Unsynchronized) {
        saveSettings();
    }
    QPushButton* sndr = qobject_cast<QPushButton*>(sender());
    if (sndr == ui->_okPb) {
        QDialog::accept();
    }
}



// Adds recipient
void SettingsDialog::addRecipient(const QString &name, const QString &code,
                                  const QString &host, int port, const QString &user, const QString &pass,
                                  const QString &archiveDir, const QString &olDir, const QString &cdDir, const QString &regDir,
                                  const QString &recipients, const QString &subject, const QString &letter,
                                  bool def)
{
    MaskshopInfo * maskshop = new MaskshopInfo(name, code, host, port, user, pass, archiveDir, olDir, cdDir, regDir, recipients, subject, letter, def, this);
    foreach (QLineEdit * lineEdit, maskshop->findChildren<QLineEdit *>()) {
        connect(lineEdit, SIGNAL(textChanged(QString)), this, SLOT(updateStatus()));
    }
    connect(maskshop->findChild<QTextEdit *>("letterTe"), SIGNAL(textChanged()), this, SLOT(updateStatus()));
    ui->tabWidget->addTab(maskshop, maskshop->name());
}



// Clears layout
void SettingsDialog::clearLayout(QLayout *layout)
{
    QLayoutItem *item;
    while((item = layout->takeAt(0))) {
        if (item->layout()) {
            clearLayout(item->layout());
        } else if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
}


// Loads settings from configuration file
void SettingsDialog::loadSettings()
{
    QCoreApplication* app = QApplication::instance();

    QSettings currentConfig(app->property("currentConfig").toString(), QSettings::NativeFormat, this);

    ui->_fsRootLe->setText(currentConfig.value("fs_root").toString());
    ui->_svnRootLe->setText(currentConfig.value("svn_root").toString());
    ui->_svnIgnoreLe->setText(currentConfig.value("svn_ignore").toStringList().join(", "));
    ui->_selfCodeLe->setText(currentConfig.value("self_code").toString());
    ui->_mailUserLe->setText(currentConfig.value("mail_username").toString());
    ui->_mailPassLe->setText(currentConfig.value("mail_password").toString());

    foreach(QString maskshopGroup, currentConfig.childGroups().filter(QRegExp("Maskshop_\\d+"))) {
        currentConfig.beginGroup(maskshopGroup);
        addRecipient(currentConfig.value("name").toString(), currentConfig.value("code").toString(),
                     currentConfig.value("host").toString(), currentConfig.value("port").toInt(), currentConfig.value("user").toString(), currentConfig.value("pass").toString(),
                     currentConfig.value("archiveDir").toString(), currentConfig.value("olDir").toString(), currentConfig.value("cdDir").toString(), currentConfig.value("regDir").toString(),
                     currentConfig.value("recipients").toString(), currentConfig.value("subject").toString(), currentConfig.value("letter").toString(),
                     currentConfig.value("default").toBool());
        currentConfig.endGroup();
    }

    // Default config
    if (currentConfig.fileName() == app->property("defaultConfig").toString()) {

        ui->_fsRootLe->setReadOnly(true);
        ui->_svnRootLe->setReadOnly(true);
        ui->_svnIgnoreLe->setReadOnly(true);
        ui->_selfCodeLe->setReadOnly(true);
        ui->_mailUserLe->setReadOnly(true);
        ui->_mailPassLe->setReadOnly(true);

        foreach (MaskshopInfo * maskshop, ui->tabWidget->findChildren<MaskshopInfo *>()) {
            maskshop->setReadonly(true);
        }
        ui->tabWidget->setTabsClosable(false);
        ui->tabWidget->cornerWidget(Qt::TopLeftCorner)->setEnabled(false);

        ui->_defaultPb->setEnabled(false);
    }

    updateStatus();
}

void SettingsDialog::onPlusButtonClicked(bool)
{
    MaskshopInfo * newTab = new MaskshopInfo();
    ui->tabWidget->addTab(newTab, "Новый маскшоп");
    ui->tabWidget->setCurrentWidget(newTab);

    updateStatus();
}

// Do, when status is changed
void SettingsDialog::onStatusChanged(Status status)
{
    QCoreApplication* app = QApplication::instance();
    switch(status) {
        case Synchronised:
            this->setWindowTitle(app->applicationName().append(" ").append(app->applicationVersion().append(" - settings")));
            break;
        default:
        {
            this->setWindowTitle(app->applicationName().append(" ").append(app->applicationVersion().append(" - settings *")));
            break;
        }
    }
}

void SettingsDialog::onTabCloseRequested(int index)
{
    QCoreApplication* app = QApplication::instance();
    QString title = app->applicationName().append(" ").append(app->applicationVersion().append(" - warning"));
    QString text = tr("Вы действительно хотите удалить маскшоп?");
    QMessageBox::StandardButton ret = QMessageBox::warning(this, title, text, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (ret == QMessageBox::Yes)
        delete ui->tabWidget->widget(index);

    if(ui->tabWidget->currentIndex() < 0)
        onPlusButtonClicked(true);

    updateStatus();
}

// Reimplementation of QDialog reject() slot
void SettingsDialog::reject()
{
    if (_status == Unsynchronized) {
        QCoreApplication* app = QApplication::instance();
        QString title = app->applicationName().append(" ").append(app->applicationVersion().append(" - warning"));
        QString text = tr("Некоторые настройки были изменены. Выйти без сохранения?");
        QMessageBox::StandardButton ret = QMessageBox::warning(this, title, text, QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes);
        if (ret == QMessageBox::Yes) {
            QDialog::reject();
        }
    } else {
        QDialog::reject();
    }
}



// Saves settings to configuration file
void SettingsDialog::saveSettings()
{
    QSettings config(QCoreApplication::instance()->property("currentConfig").toString(), QSettings::NativeFormat, this);

    QCoreApplication* app = QApplication::instance();
    if (config.isWritable()) {

        config.setValue("fs_root", ui->_fsRootLe->text().trimmed());
        config.setValue("svn_root", ui->_svnRootLe->text().trimmed());
        config.setValue("svn_ignore", ui->_svnIgnoreLe->text().trimmed().split(QRegExp(",\\s*")));
        config.setValue("self_code", ui->_selfCodeLe->text().trimmed());
        config.setValue("mail_username", ui->_mailUserLe->text().trimmed());
        config.setValue("mail_password", ui->_mailPassLe->text().trimmed());

        int groupIdentifier = 0;
        foreach (MaskshopInfo * maskshop, ui->tabWidget->findChildren<MaskshopInfo *>()) {
            maskshop->save();
            config.beginGroup("Maskshop_" + QString::number(++groupIdentifier));
            config.setValue("name", maskshop->name());
            config.setValue("code", maskshop->gpg());
            config.setValue("host", maskshop->host());
            config.setValue("port", maskshop->port());
            config.setValue("user", maskshop->username());
            config.setValue("pass", maskshop->password());
            config.setValue("archiveDir", maskshop->archiveDir());
            config.setValue("olDir", maskshop->olDir());
            config.setValue("cdDir", maskshop->cdDir());
            config.setValue("regDir", maskshop->regDir());
            config.setValue("recipients", maskshop->recipients());
            config.setValue("subject", maskshop->subject());
            config.setValue("letter", maskshop->letter());
            config.setValue("default", int(maskshop->_default));
            config.endGroup();
        }
        while (config.childGroups().contains("Maskshop_" + QString::number(++groupIdentifier)))
            config.remove("Maskshop_" + QString::number(groupIdentifier));

        if (config.status() == QSettings::NoError) {
            QString title = app->applicationName().append(" ").append(app->applicationVersion().append(" - info"));
            QString text = tr("Изменения вступят в силу после перезапуска приложения");
            QMessageBox::information(this, title, text);
            updateStatus();
        } else {
            QString title = app->applicationName().append(" ").append(app->applicationVersion().append(" - error"));
            QString text = tr("При сохранении изменений произошла ошибка.");
            QMessageBox::critical(this, title, text);
        }
    } else {
        QString title = app->applicationName().append(" ").append(app->applicationVersion().append(" - error"));
        QString text = tr("Невозможно сохранить изменения, конфигурационный файл \")").append(config.fileName()).append(tr("\" недоступен для записи"));
        QMessageBox::critical(this, title, text);
    }
}



// Sets default values to configuration file
void SettingsDialog::setDefaults()
{
    QCoreApplication* app = QApplication::instance();
    QString title = app->applicationName().append(" ").append(app->applicationVersion().append(" - warning"));
    QString text = tr("Вы действительно хотите восстановить параметры по умолчанию?");
    QMessageBox::StandardButton ret = QMessageBox::warning(this, title, text, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (ret == QMessageBox::Yes) {
        QCoreApplication* app = QApplication::instance();
        QString defaultConfig = app->property("defaultConfig").toString();
        QString userConfig = app->property("userConfig").toString();

        if (userConfig != defaultConfig) {
            QFile::remove(userConfig);
            QFile::copy(defaultConfig, userConfig);
            ui->tabWidget->clear();
            loadSettings();

            QString title = app->applicationName().append(" ").append(app->applicationVersion().append(" - info"));
            QString text = tr("Изменения вступят в силу после перезапуска приложения");
            QMessageBox::information(this, title, text);
            updateStatus();
        }
    }
}



// Sets status to "status"
void SettingsDialog::setStatus(Status status)
{
    if (_status != status) {
        _status = status;
        emit statusChanged(status);
    }
}



// Shows tooltip messages
void SettingsDialog::showTooltip()
{
    QPushButton* pb = qobject_cast<QPushButton*>(sender());
    if (pb) {
        QCoreApplication* app = QApplication::instance();
        QString title = app->applicationName().append(" ").append(app->applicationVersion().append(" - info"));
        QString text;
        if (pb == ui->_fsRootPb) {
            text = tr("Корневая директория, относительно которой выполняется навигация по файловой системе.\ndefault = /workarea/otd15/users");
        } else if (pb == ui->_svnRootPb) {
            text = tr("Корневой url, относительно которого выполняется навигация по SVN.\ndefault = http://mikron-subversion/svn/reticle/trunk");
        } else if (pb == ui->_svnIgnorePb) {
            text = tr("Список директорий, игнорирующихся при формировании иерархии репозитория в окне навигации по SVN.\ndefault = Development, SCRIPTS, topology");
        }
        QMessageBox::information(this, title, text);
    }
}



// Returns settings dialog status
SettingsDialog::Status SettingsDialog::status()
{
    return _status;
}



// Updates status
void SettingsDialog::updateStatus()
{
    QSettings config(QCoreApplication::instance()->property("currentConfig").toString(), QSettings::NativeFormat, this);

    bool unsync = false;
    if (ui->_fsRootLe->text().trimmed() != config.value("fs_root").toString() ||
        ui->_svnRootLe->text().trimmed() != config.value("svn_root").toString() ||
        ui->_svnIgnoreLe->text().trimmed().split(QRegExp(",\\s*")) != config.value("svn_ignore").toStringList() ||
        ui->_selfCodeLe->text().trimmed() != config.value("self_code").toString() ||
        ui->_mailUserLe->text().trimmed() != config.value("mail_username").toString() ||
        ui->_mailPassLe->text().trimmed() != config.value("mail_password").toString())
    {
        unsync = true;
    }

    // Проверка маскшопов
    if (config.childGroups().filter(QRegExp("Maskshop_\\d+")).size() != ui->tabWidget->findChildren<MaskshopInfo *>().size())
    {
        unsync = true;
    }
    else
    {
        foreach (QString maskshopGroup, config.childGroups().filter(QRegExp("Maskshop_\\d+"))) {
            config.beginGroup(maskshopGroup);
            QString name = config.value("name").toString();
            MaskshopInfo * maskshop;
            foreach(MaskshopInfo * ms, ui->tabWidget->findChildren<MaskshopInfo *>())
                if (ms->name() == name) maskshop = ms;
            if (maskshop->findChild<QLineEdit *>("nameLe")->text() != config.value("name").toString() ||
                maskshop->findChild<QLineEdit *>("gpgLe")->text() != config.value("code").toString() ||
                maskshop->findChild<QLineEdit *>("hostLe")->text() != config.value("host").toString() ||
                maskshop->findChild<QLineEdit *>("portLe")->text().toInt() != config.value("port").toInt() ||
                maskshop->findChild<QLineEdit *>("userLe")->text() != config.value("user").toString() ||
                maskshop->findChild<QLineEdit *>("passLe")->text() != config.value("pass").toString() ||
                maskshop->findChild<QLineEdit *>("archiveDirLe")->text() != config.value("archiveDir").toString() ||
                maskshop->findChild<QLineEdit *>("olDirLe")->text() != config.value("olDir").toString() ||
                maskshop->findChild<QLineEdit *>("cdDirLe")->text() != config.value("cdDir").toString() ||
                maskshop->findChild<QLineEdit *>("regDirLe")->text() != config.value("regDir").toString() ||
                maskshop->findChild<QLineEdit *>("recipientsLe")->text() != config.value("recipients").toString() ||
                maskshop->findChild<QLineEdit *>("subjectLe")->text() != config.value("subject").toString() ||
                maskshop->findChild<QTextEdit *>("letterTe")->toPlainText() != config.value("letter").toString())
            {
                unsync = true;
            }
            config.endGroup();
        }
    }

    if (unsync) {
        setStatus(Unsynchronized);
    } else {
        setStatus(Synchronised);
    }
}



void SettingsDialog::on__selfCodePb_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Укажите файл ключа", QDir::homePath(), "*.asc *.key *.txt");
    if (!fileName.isEmpty())
    {
        QProcess p;
        p.start("gpg --import " + fileName);
        p.waitForFinished(-1);

        QString output = p.readAllStandardError();

        ui->_selfCodeLe->setText(output.mid(output.indexOf(QRegExp("[A-F0-9]{8}")), 8));
    }
}

void SettingsDialog::on__keyOpfsh_toggled(bool checked)
{
    QApplication::instance()->setProperty("keyOpfsh", checked);
}
