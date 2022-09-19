#include <QComboBox>
#include <QCommonStyle>
#include <QCoreApplication>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include "include/mainwindow.h"
#include "ui_mainwindow.h"

// Constructor 1
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->_splitter->setStretchFactor(1, 1);
    ui->_launchPb->setIcon(QCommonStyle().standardIcon(QStyle::SP_DialogOkButton));
    ui->_launchPb->setIconSize(QSize(16,16));
    ui->_mailCb->setEnabled(false);
    ui->_checkCb->setEnabled(false);
    _appInfoDialog = new AppInfoDialog(this);
    _browseSvnDialog = 0;

    QCoreApplication* app = QApplication::instance();

    // Create user configuration file
    QString defaultConfig = app->property("defaultConfig").toString();
    QString userConfig = app->property("userConfig").toString();
    QDir userDir = QFileInfo(userConfig).dir();
    userDir.mkpath(userDir.path());
    if (!QFile::exists(userConfig)) {
        if (!QFile::copy(defaultConfig, userConfig)) {
            QString title = app->applicationName().append(" ").append(app->applicationVersion().append(" - warning"));
            QString text = tr("Не удалось создать пользовательский конфигурационный файл \"").append(userConfig).append(tr("\". Ручная конфигурация приложения невозможна."));
            QMessageBox::warning(this, title, text);
            app->setProperty("currentConfig", defaultConfig);
        }
    }

    QSettings currentConfig(app->property("currentConfig").toString(), QSettings::NativeFormat, this);

    // Default recipients
    foreach(QString recipientGroup, currentConfig.childGroups().filter(QRegExp("Maskshop_\\d+"))) {
        currentConfig.beginGroup(recipientGroup);
        if (currentConfig.value("default").toString() == "1") {
            addRecipient(currentConfig.value("name").toString());
        }
        currentConfig.endGroup();
    }

    connect(ui->_settigsA, SIGNAL(triggered()), this, SLOT(showSettingsDialog()));
    connect(ui->_orderLe, SIGNAL(textChanged(QString)), this, SLOT(onOrderPathChanged(QString)));
    connect(ui->_removeRecipientPb, SIGNAL(clicked()), this, SLOT(removeRecipient()));
    connect(ui->_addRecipientPb, SIGNAL(clicked()), this, SLOT(addRecipient()));
    connect(ui->_browsePb, SIGNAL(clicked()), this, SLOT(onBrowsePbClicked()));
    connect(ui->_launchPb, SIGNAL(clicked()), this, SLOT(launch()));
    connect(ui->_filesCb, SIGNAL(clicked()), this, SLOT(toggleFiles()));
    connect(ui->_svnPb, SIGNAL(clicked()), this, SLOT(onSvnPbClicked()));
    connect(ui->_aboutProgrammAction, SIGNAL(triggered()), _appInfoDialog, SLOT(exec()));

    updateHelpMenu();
}



// Destructor
MainWindow::~MainWindow()
{
    delete ui;
}



// Adds recipient with name "name"
void MainWindow::addRecipient(const QString &name)
{
    if (ui->_recipientsLay->count() < 5) {

        QSettings currentConfig(QCoreApplication::instance()->property("currentConfig").toString(), QSettings::NativeFormat, this);

        QComboBox* cmb = new QComboBox;
        cmb->setFixedSize(150, 25);
        cmb->addItem(QString());
        foreach(QString group, currentConfig.childGroups()) {
            if (QRegExp("Maskshop_\\d+").exactMatch(group)) {
                cmb->addItem(currentConfig.value(QString(group).append("/name")).toString());
            }
        }
        if (!name.isNull()) {
            cmb->setCurrentIndex(cmb->findText(name));
        }

        ui->_recipientsLay->addWidget(cmb);
    }
}

// Returns true if configuration is valid, otherwise returns false
bool MainWindow::isConfigurationValid()
{
    QStringList errMsgs;

    if (ui->_orderLe->text().isEmpty()) {
        errMsgs << "Не заполнено обязательное поле \"Файл-задание\"";
    } else {
        QProcess p;
        QString programm = "svn info " + ui->_orderLe->text();
        p.start(programm);
        if (p.waitForStarted()) {
            if (p.waitForFinished(-1)) {
                if (p.exitCode() != 0 && !QFile::exists(ui->_orderLe->text())) {
                    errMsgs << "Отсутсвует файл указанный в поле \"Файл-задание\"";
                }
            }
        }
    }

    QStringList recipients;
    for(int i = 0; i < ui->_recipientsLay->count(); i++) {
        QComboBox* recipientCmb = qobject_cast<QComboBox*>(ui->_recipientsLay->itemAt(i)->widget());
        if (recipientCmb) {
            recipients << recipientCmb->currentText().trimmed();
        }
    }
    if (recipients.isEmpty()) {
        errMsgs << "список реципиентов пуст";
    }
    if (recipients.contains(QString())) {
        errMsgs << "список реципиентов содержит пустые имена";
    }
    if (recipients.removeDuplicates() != 0) {
        errMsgs << "список реципиентов содержит повторяющиеся имена";
    }
    if (_fileCbs.isEmpty()) {
        errMsgs << "Cписок файлов для шифрования пуст";
    } else {
        bool checked = false;
        foreach (QCheckBox* cb, _fileCbs) {
            if (cb->isChecked()) {
                checked = true;
            }
        }
        if (!checked) {
            errMsgs << "Не выбраны файлы для шифрования";
        }
    }

    if (errMsgs.isEmpty()) {
        return true;
    } else {
        QCoreApplication* app = QApplication::instance();
        QString title = app->applicationName().append(" ").append(app->applicationVersion().append(" - error"));
        QString text = "Невозможно выполнить запуск. Конфигурация содержит следующие ошибки:";
        foreach(QString errMsg, errMsgs) {
            text.append("\n- ").append(errMsg);
        }
        QMessageBox::critical(this, title, text);
        return false;
    }
}



// Launches encryption
void MainWindow::launch()
{
    if (isConfigurationValid()) {
        QSettings currentConfig(QCoreApplication::instance()->property("currentConfig").toString(), QSettings::NativeFormat, this);

        // Maskshop
        QString maskshop;
        if (!ui->_recipientsLay->isEmpty()) {
            QComboBox* recipientCmb = qobject_cast<QComboBox*>(ui->_recipientsLay->itemAt(0)->widget());
            if (recipientCmb) {
                maskshop = recipientCmb->currentText().trimmed();
            }
        }

        // Recipients
        QStringList recipients;
        for(int i = 0; i < ui->_recipientsLay->count(); i++) {
            QComboBox* recipientCmb = qobject_cast<QComboBox*>(ui->_recipientsLay->itemAt(i)->widget());
            if (recipientCmb) {
                foreach(QString recipientGroup, currentConfig.childGroups().filter(QRegExp("Maskshop_\\d+"))) {
                    currentConfig.beginGroup(recipientGroup);
                    if (currentConfig.value("name").toString() == recipientCmb->currentText().trimmed()) {
                        recipients << currentConfig.value("code").toString();
                        currentConfig.endGroup();
                        break;
                    }
                    currentConfig.endGroup();
                }
            }
        }

        // Files
        QStringList files;
        foreach (QCheckBox *cb, _fileCbs) {
            if (cb->isChecked()) {
                foreach (QString file, _filePaths) {
                    if (file.contains(cb->text())) {
                        files << file;
                    }
                }
            }
        }
        _outputDialog = new OutputDialog(this);
        _flow = new Flow(_filePath, maskshop, recipients, files, ui->_ftpCb->isChecked() + ui->_mailCb->isChecked(), ui->_checkCb->isChecked(), this);
        connect(_flow, SIGNAL(updateProgress(int)), _outputDialog, SLOT(setProgressValue(int)));
        connect(_flow, SIGNAL(infoMsg(QString)), _outputDialog, SLOT(infoMessage(QString)));
        connect(_flow, SIGNAL(errorMsg(QString)), _outputDialog, SLOT(errorMessage(QString)));
    }
}

// Launches document
void MainWindow::launchDoc()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        QString name = action->text();
        QSettings commonConfig(QFileInfo(QApplication::applicationDirPath()).dir().path() + "/cfg/common.conf", QSettings::NativeFormat);
        foreach(QString docGroup, commonConfig.childGroups().filter(QRegExp("^Doc_\\d+$"))) {
            commonConfig.beginGroup(docGroup);
            if (commonConfig.value("name").toString() == name) {
                QString path = commonConfig.value("path").toString().replace("<APP_DIR>", QFileInfo(QApplication::applicationDirPath()).dir().path());
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
                break;
            }
            commonConfig.endGroup();
        }
    }
}

// Do, when filePath changed
void MainWindow::onOrderPathChanged(const QString filePath)
{
    if (_filePath != filePath) {
        _filePath = filePath;
        onSelectionChanged();
    }
}

// Do, when current item of file system navigation model is changed
void MainWindow::onSelectionChanged()
{
    qDeleteAll(_fileCbs);
    _fileCbs.clear();
    _filePaths.clear();

    QStringList files;
    QString dir = QFileInfo(_filePath).path();

    if (QFile::exists(dir)) {
        QSettings orderConfig(_filePath, QSettings::NativeFormat);
        QString mwp = orderConfig.value("mpw").toString();
        QString lrType = orderConfig.value("lrType").toString();
        foreach(QString maskGroup, orderConfig.childGroups().filter(QRegExp("^Mask_\\d+$"))) {
            QString layer1 = orderConfig.value(QString(maskGroup).append("/layer1")).toString();
            QString layer2 = orderConfig.value(QString(maskGroup).append("/layer2")).toString();
            QString barcode = orderConfig.value(QString(maskGroup).append("/barcode")).toString();
            bool psm = orderConfig.value(QString(maskGroup).append("/psm")).toBool();

            if (lrType == "MLR") {
                files << QString(mwp) + "_" + layer1 + "_" + layer2 + "_" + barcode[9] + barcode[10] + barcode[11] + ".oas.gz";
                _filePaths << dir + "/OUT/" + QString(mwp) + "_" + layer1 + "_" + layer2 + "_" + barcode[9] + barcode[10] + barcode[11] + ".oas.gz";
                if (psm) {
                    files << QString(mwp) + "_" + layer1 + "_" + layer2 + "_" + "PSM" + ".oas.gz";
                    _filePaths << dir + "/OUT/" + QString(mwp) + "_" + layer1 + "_" + layer2 + "_" + "PSM" + ".oas.gz";
                }
            } else if (lrType == "SLR") {
                files << QString(mwp) + "_" + layer1 + "_" + barcode[9] + barcode[10] + barcode[11] + ".oas.gz";
                 _filePaths << dir + "/OUT/" + QString(mwp) + "_" + layer1 + "_" + barcode[9] + barcode[10] + barcode[11] + ".oas.gz";
                if (psm) {
                    files << QString(mwp) + "_" + layer1 + "_" + "PSM" + ".oas.gz";
                    _filePaths << dir + "/OUT/" + QString(mwp) + "_" + layer1 + "_" + "PSM" + ".oas.gz";
                }
            }
        }
    } else {
        QProcess p;
        QString programm = QString("svn list ") + dir + "/OUT" ;
        p.start(programm);
        if (p.waitForStarted()) {
            if (p.waitForFinished()) {
                if (p.exitStatus() == 0) {
                    QString output = p.readAllStandardOutput();
                    foreach (QString file, output.split("\n")) {
                        if (!file.isEmpty()) {
                            files << file;
                            _filePaths << dir + "/OUT/" + file;
                        }
                    }
                }
            }
        }
    }

    foreach(QString file, files) {
        QCheckBox* cb = new QCheckBox(file, this);
        cb->setFixedHeight(25);
        cb->setChecked(true);
        connect(cb, SIGNAL(toggled(bool)), this, SLOT(updateFilesCb()));
        _fileCbs << cb;
    }

    updateFilesCb();
    showFiles();
}

// Clicked browse pb
void MainWindow::onBrowsePbClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Открыть...", QDir::currentPath(), "Configuration files (*.conf);;All files (*)");
    if (!filePath.isEmpty()) {
        ui->_orderLe->clear();
        ui->_orderLe->setText(filePath);
    }
}

// Clicked svn pb
void MainWindow::onSvnPbClicked()
{
    if (!_browseSvnDialog) {
        _browseSvnDialog = new BrowseSvnDialog(this);
        _browseSvnDialog->setWindowTitle("Открыть из SVN...");
    }
    if (_browseSvnDialog->exec() == QDialog::Accepted) {
        QString filePath = _browseSvnDialog->path();
        if (!filePath.isEmpty()) {
            ui->_orderLe->clear();
            ui->_orderLe->setText(filePath);
        }
    }
}

// Removes last recipient, if recipient list contains more than one element
void MainWindow::removeRecipient()
{
    if (ui->_recipientsLay->count() > 0) {
        QWidget* w = ui->_recipientsLay->takeAt(ui->_recipientsLay->count() - 1)->widget();
        delete w;
    }
}



// Do on resize event
void MainWindow::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    showFiles();
}



// Shows files of directory with path "dirPath"
void MainWindow::showFiles()
{
    // Clear layout
    QLayoutItem *item;
    while ((item = ui->_filesL->takeAt(0))) {}

    // Fill layout
    int rowCount = (ui->_filesSa->viewport()->height() - 45) / (25 + 6);
    int row = 0;
    int col = 0;
    foreach(QCheckBox* fileCb, _fileCbs) {
        ui->_filesL->addWidget(fileCb, row, col);
        if (row == rowCount - 1) {
            row = 0;
            col++;
        } else {
            row++;
        }
    }
}



// Shows settings dialog
void MainWindow::showSettingsDialog()
{
    SettingsDialog dialog(this);
    dialog.exec();
}



// Toggles files
void MainWindow::toggleFiles()
{
    bool checked = false;
    if (ui->_filesCb->isChecked() || ui->_filesCb->checkState() == Qt::PartiallyChecked) {
        checked = true;
    }
    foreach(QCheckBox* cb, _fileCbs) {
        cb->setChecked(checked);
    }
}



// Updates files checkbox
void MainWindow::updateFilesCb()
{
    bool checked = false;
    bool unchecked = false;

    foreach(QCheckBox* cb, _fileCbs) {
        cb->isChecked() ? checked = true : unchecked = true;
    }

    if (checked && unchecked) {
        ui->_filesCb->setCheckState(Qt::PartiallyChecked);
    } else if (checked) {
        ui->_filesCb->setChecked(true);
    } else {
        ui->_filesCb->setChecked(false);
    }

    ui->_filesCb->repaint();
}

// Updates help menu
void MainWindow::updateHelpMenu()
{
    QSettings commonConfig(QFileInfo(QApplication::applicationDirPath()).dir().path().append("/cfg/common.conf"), QSettings::IniFormat);
    foreach(QString docGroup, commonConfig.childGroups().filter(QRegExp("^Doc_\\d+$"))) {
        QString name = commonConfig.value(docGroup + "/name").toString();
        QAction* action = new QAction(name, ui->_helpMenu);
        connect(action, SIGNAL(triggered()), this, SLOT(launchDoc()));
        ui->_helpMenu->insertAction(ui->_helpMenu->actions().first(), action);
    }
}

void MainWindow::on__ftpCb_stateChanged(int arg1)
{
    ui->_mailCb->setEnabled(arg1);
    ui->_mailCb->setChecked(arg1);
    ui->_checkCb->setEnabled(arg1);
    ui->_checkCb->setChecked(arg1);
}
