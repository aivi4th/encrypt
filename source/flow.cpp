#include "include/flow.h"
#include <QMessageBox>
#include <QPushButton>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QSettings>
#include <QDateTime>
#include <QApplication>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include "include/common.h"
#include "include/outputdialog.h"
#include "include/notifier.h"

// Constructor
Flow::Flow(const QString &orderPath, const QString &maskShop, const QStringList &recipients, const QStringList &files, int mode, bool check, QObject *parent) :
    QObject(parent),
    _mode(mode), _check(check), _maskShop(maskShop)
{
    emit updateProgress(0);
    _startDir = QDir::currentPath();
    _workDir = QDir::currentPath() + "/" + maskShop;
    _startTime = QDateTime::currentDateTime().toString("yyyyddMMhhmm");
    _error = false;
    QDir().mkdir(_workDir);

    connect(this, SIGNAL(endRun()), this, SLOT(deleteLater()));
    connect(this, SIGNAL(endEncriptyon()), this, SLOT(ftpSend()));

    initActions(orderPath, maskShop, recipients, files);
}

// Destcuctor
Flow::~Flow()
{
    _actionsFuture.cancel();
    _actionsFuture.waitForFinished();
    _actionsWatcher.cancel();
    _actionsWatcher.deleteLater();
}

// Archiving files
QString Flow::archivingFiles()
{
    QProcess p;
    QString programm = "tar -cvf " + _mpw + "_" + _startTime + ".tar " + _workFiles.join(" ");
    p.start(programm);
    if (p.waitForStarted()) {
        if (p.waitForFinished(-1)) {
            if (p.exitStatus() == 0) {
                emit infoMsg("creating archiv:" + _mpw + "_" + _startTime + ".tar");
                return QString(_workDir + "/" + _mpw + "_" + _startTime + ".tar");
            }
        }
    }
    return QString();
}

// Actions
void Flow::actions(const QString &orderPath, const QString &maskShop, const QStringList &recipients, const QStringList &files)
{
    QDir::setCurrent(_workDir);
    if (!_mpw.isEmpty()) {
        loadingFiles(files);
        if (maskShop != "TOPPAN") {
            QStringList arch;
            arch << archivingFiles();
            _encryptedFiles << encriptyonFiles(arch, recipients);
        } else {
            _encryptedFiles << encriptyonFiles(_workFiles, recipients);
        }
        emit updateProgress(_mode ? 40 : 80);
        postActions(orderPath);
        emit updateProgress(_mode ? 50 : 100);
        emit infoMsg("");

        if (!_resultsData.isEmpty()) {
            QString info = "<table border=\"1\" cellpadding=\"5\">";
            foreach (QStringList list, _resultsData) {
                info += "<tr>";
                foreach (QString string, list) {
                    info += "<td>" + string + "</td>";
                }
                info += "</tr>";
            }
            info += "</table>";
            emit infoMsg(info);
            emit infoMsg("");
            emit infoMsg("Шифрование завершено успешно.");
        } else {
            emit errorMsg("Шифрование завершено с ошибками.");
            _error = true;
        }
    }

    if (_mode) {
        QDir::setCurrent(_startDir);
        emit endEncriptyon();
    }
    else {
        QDir::setCurrent(_startDir);
        emit endRun();
    }
}

// Encriptyon files
QStringList Flow::encriptyonFiles(const QStringList &files, const QStringList &recipients)
{
    QStringList encriptedFiles;
    for (int i = 0; i < files.count(); i++) {
        emit updateProgress(_mode ? (40 + (int)(30/files.count() * i))/2 : 40 + (int)(30/files.count() * i));
        if (QFile::exists(files[i])) {
            QProcess p;
            QString programm = "gpg";
            QStringList args;
            args << "--always-trust";
            args << "--yes";
            args << "-e";
            args << "-o";
            args << files[i] + ".gpg";
            foreach (QString recipient, recipients) {
                args << "-r";
                args << recipient;
            }
            if (QApplication::instance()->property("keyOpfsh").toBool())
            {
                args << "-r";
                args << QSettings(QApplication::instance()->property("currentConfig").toString(), QSettings::IniFormat).value("self_code").toString();
            }
            args << files[i];

            p.start(programm,args);
            if (p.waitForStarted()) {
                if (p.waitForFinished(-1)) {
                    if (p.exitCode() == 0) {
                        emit infoMsg("encrypt: " + files[i] + ".gpg");
                        encriptedFiles << files[i] + ".gpg";
                    }
                }
            }
        }
    }
    return encriptedFiles;
}

// Load files
void Flow::loadingFiles(const QStringList &files)
{
    _workFiles.clear();
    for (int i = 0; i < files.count(); i++) {
        emit updateProgress(_mode ? (10 + (int)(50/files.count() * i))/2 : 10 + (int)(50/files.count() * i));
        if (QFile::exists(files[i])) {
            QFile::copy(files[i],_workDir + "/" + QFileInfo(files[i]).fileName());
            _workFiles << QFileInfo(files[i]).fileName();
            emit infoMsg("copying: " + files[i]);
        } else {
            QProcess p;
            QString programm = "svn export " + files[i] + " " + _workDir + "/" + QFileInfo(files[i]).fileName();
            p.start(programm);
            if (p.waitForStarted()) {
                if (p.waitForFinished(-1)) {
                    if (p.exitStatus() == 0) {
                        emit infoMsg("exporting: " + files[i]);
                        _workFiles << QFileInfo(files[i]).fileName();
                    }
                }
            }
        }
    }
}

void Flow::loadOtherFiles(const QStringList &files)
{
    _otherFiles.clear();
    QDir odr(files.front().section("/", 0, -3) + "/DOC/OrderListing/");
    QStringList filters = {"*OL*", "*CD*", "*REG*"};
    if (!odr.entryList(filters).isEmpty()) {
        foreach (QString otherFile, odr.entryList(filters))
            _otherFiles << odr.path() + "/" + otherFile;
    }
    else {
        QProcess p;
        QString programm = "svn export " + odr.path() + " " + _workDir + " --force";
        p.start(programm);
        if (p.waitForStarted()) {
            if (p.waitForFinished(-1)) {
                if (p.exitCode() == 0) {
                    foreach(QString otherFile, QString(p.readAllStandardOutput()).section("\n",1,-3).split(QRegExp("\\n?A\\s+")))
                        if (!otherFile.isEmpty()) _otherFiles << _workDir + "/" + otherFile;
                }
            }
        }
    }
}

void Flow::catOrderFile(const QString &orderPath)
{
    QDir::setCurrent(_workDir);
    emit updateProgress(_mode ? 3 : 5);
    if (QFile::exists(orderPath)) {
        QSettings orderConfig(orderPath, QSettings::NativeFormat);
        _mpw = orderConfig.value("mpw").toString();
    } else {
        QProcess p;
        QString programm = "svn cat " + orderPath ;
        p.start(programm);
        if (p.waitForStarted()) {
            if (p.waitForFinished(-1)) {
                if (p.exitCode() == 0) {
                    _mpw = Functions::getConfigValue(p.readAllStandardOutput(), "General/mpw").toString();
                }
            }
        }
    }
}

// Select value in database
QVariant Flow::selectValue(const QString &table, const QString &column, const QString &condition)
{
    QString queryText = "SELECT `" + column + "` FROM `" + table + "` WHERE " +  condition;
    QSqlQuery query;
    if (query.exec(queryText)) {
       QStringList values;
       while (query.next()) {
           values << query.value(0).toString();
       }
       return QVariant(values);
    } else {
       emit errorMsg("Error select: " + queryText);
    }
    return QVariant();
}

// Insert value in database
void Flow::insertValue(const QString &table, const QStringList &columns, const QStringList &values)
{
    QString queryText;
    queryText = "INSERT INTO `" + table + "` (`" + columns.join("`, `") + "`) VALUES ('" + values.join("', '") + "')";

    QSqlQuery query;
    if (query.exec(queryText)) {
        emit infoMsg(queryText);
    } else {
        emit errorMsg(queryText);
    }
}

// Init actions to thread
void Flow::initActions(const QString &orderPath, const QString &maskShop, const QStringList &recipients, const QStringList &files)
{
    catOrderFile(orderPath);

    bool chfl = true;
    if (_check) {
        loadOtherFiles(files);
        chfl = check();
    }


    if (chfl) {
        _actionsWatcher.cancel();
        _actionsWatcher.waitForFinished();
        _actionsFuture = QtConcurrent::run(this, &Flow::actions, orderPath, maskShop, recipients, files);
        _actionsWatcher.setFuture(_actionsFuture);
    }
}

// Update value in database
void Flow::updateValue(const QString &table, const QString &column, const QString &condition, const QString &value)
{
    QString queryText = "UPDATE `" + table + "` SET " + column + " = '" + value + "' WHERE " + condition;
    QSqlQuery query;
    if (query.exec(queryText)) {
        emit infoMsg(queryText);
    } else {
        emit errorMsg(queryText);
    }
}

// Post actions
void Flow::postActions(const QString &orderPath)
{
    QSettings commonConfig(QFileInfo(QApplication::applicationDirPath()).dir().path().append("/cfg/common.conf"),QSettings::NativeFormat);

    QString tz;
    QString mpw;
    QString lrType;
    if (QFile::exists(orderPath)) {
        QSettings orderConfig(orderPath, QSettings::NativeFormat);
        mpw = orderConfig.value("mpw").toString();
        tz = orderConfig.value("tz").toString();
        lrType = orderConfig.value("lrType").toString();
    } else {
        QProcess p;
        QString programm = "svn cat " + orderPath ;
        p.start(programm);
        if (p.waitForStarted()) {
            if (p.waitForFinished(-1)) {
                if (p.exitCode() == 0) {
                    QString orderContent = p.readAllStandardOutput();
                    mpw = Functions::getConfigValue(orderContent, "General/mpw").toString();
                    tz = Functions::getConfigValue(orderContent, "General/tz").toString();
                    lrType = Functions::getConfigValue(orderContent, "General/lrType").toString();
                }
            }
        }
    }

    QSqlDatabase database = QSqlDatabase::addDatabase("QMYSQL");
    database.setHostName(commonConfig.value("Sql/host").toString());
    database.setDatabaseName(commonConfig.value("Sql/database").toString());
    QString user = "py_encrypt";
    QString password = "SBA52B9nyfAzeC8n";
    database.setUserName(user);
    database.setPassword(password);
    if (database.open()) {
        QString mpwId = selectValue("UiMpw","Id", "Name='" + mpw + "'").toString();
        QString tzId = selectValue("UiTz","Id","MpwId='" + mpwId + "' AND Name='" + tz + "'").toString();

        if (tzId.isEmpty()) {
            emit errorMsg("Не найден Id в таблице \'UiTz\' для текущих MPW и TZ указанных в файле-задании");
            database.close();
            return;
        }

        if (QFile::exists(orderPath)) {
            QSettings orderConfig(orderPath, QSettings::NativeFormat);
            QStringList maskGroups = orderConfig.childGroups().filter(QRegExp("^Mask_\\d+$"));
            foreach (QString maskGroup, maskGroups) {
                QString layer1 = orderConfig.value(QString(maskGroup).append("/layer1")).toString();
                QString layer2 = orderConfig.value(QString(maskGroup).append("/layer2")).toString();
                QString barcode = orderConfig.value(QString(maskGroup).append("/barcode")).toString();
                bool psm = orderConfig.value(QString(maskGroup).append("/psm")).toBool();

                QString currentFileName;
                QString psmFileName;
                if (lrType == "MLR") {
                    currentFileName = mpw + "_" + layer1 + "_" + layer2 + "_" + barcode[9] + barcode[10] + barcode[11] + ".oas.gz";
                    if (psm) {
                        psmFileName = mpw + "_" + layer1 + "_" + layer2 + "_" + "PSM" + ".oas.gz";
                    }
                } else if (lrType == "SLR") {
                    currentFileName = mpw + "_" + layer1 + "_" + barcode[9] + barcode[10] + barcode[11] + ".oas.gz";
                    if (psm) {
                        psmFileName = mpw + "_" + layer1 + "_" + "PSM" + ".oas.gz";
                    }
                }

                if (_workFiles.contains(psmFileName)) {
                    QStringList data;
                    QStringList columns;
                    QStringList values;

                    columns << "UiTzId";
                    values << tzId;
                    columns << "Layer1";
                    values << layer1;
                    if (lrType == "MLR") {
                        columns << "Layer2";
                        values << layer2;
                    }

                    columns << "FileName";
                    values << psmFileName;
                    data << psmFileName;
                    data << "";
                    QString psmMd5 = Functions::fileCheckSum(psmFileName, QCryptographicHash::Md5);
                    data << psmMd5;
                    if (!psmMd5.isEmpty()) {
                        columns << "FileHash";
                        values << psmMd5;
                    }

                    QString psmTopCell;
                    QProcess p1;
                    QString programm = "calibredrv -a layout peek " + psmFileName + " -topcell";
                    p1.start(programm);
                    if (p1.waitForStarted()) {
                        if (p1.waitForFinished(-1)) {
                            if (p1.exitCode() == 0) {
                                QString output = p1.readAllStandardOutput();
                                psmTopCell = output.split("\n")[0];
                            }
                        }
                    }
                    if (!psmTopCell.isEmpty()) {
                        columns << "Topcell";
                        values << psmTopCell;
                    }
                    QString psmLayer;
                    programm = "calibredrv -a layout peek " + psmFileName + " -layers";
                    p1.start(programm);
                    if (p1.waitForStarted()) {
                        if (p1.waitForFinished(-1)) {
                            if (p1.exitCode() == 0) {
                                QString output = p1.readAllStandardOutput();
                                psmLayer = output.split(" ")[0].replace("{", "");
                            }
                        }
                    }
                    if (!psmLayer.isEmpty()) {
                        columns << "Layer";
                        values << psmLayer;
                    }
                    QStringList id = selectValue("UiPsmMask", "Id", "UiTzId='" + tzId + "' AND FileName ='" + psmFileName + "'").toStringList();
                    if (id.isEmpty()) {
                        insertValue("UiPsmMask",columns,values);
                    }
                    _resultsData << data;
                }

                if (_workFiles.contains(currentFileName)) {
                    QStringList data;
                    data << currentFileName;
                    data << barcode;
                    // Update FileName in table UiMask
                    QString fileName = selectValue("UiMask", "FileName", "UiTzId='" + tzId + "' AND Barcode ='" + barcode + "'").toString();
                    if (fileName.isEmpty()) {
                        updateValue("UiMask", "FileName", "Barcode = '" + barcode + "'", currentFileName);
                    }

                    // Update md5 in table UiMask
                    QString md5 = selectValue("UiMask", "FileHash", "UiTzId='" + tzId + "' AND Barcode ='" + barcode + "'").toString();
                    QString currentMd5 = Functions::fileCheckSum(currentFileName, QCryptographicHash::Md5);
                    data << currentMd5;
                    if (md5.isEmpty() && !currentMd5.isEmpty()) {
                        updateValue("UiMask", "FileHash", "Barcode = '" + barcode + "'", currentMd5);
                    }

                    // Update topCell in table UiMask
                    QString topCell = selectValue("UiMask", "Topcell", "UiTzId='" + tzId + "' AND Barcode ='" + barcode + "'").toString();
                    QString currentTopCell;
                    QProcess p1;
                    QString programm = "calibredrv -a layout peek " + currentFileName + " -topcell";
                    p1.start(programm);
                    if (p1.waitForStarted()) {
                        if (p1.waitForFinished(-1)) {
                            if (p1.exitCode() == 0) {
                                QString output = p1.readAllStandardOutput();
                                currentTopCell = output.split("\n")[0];
                            }
                        }
                    }
                    if (topCell.isEmpty() && !currentTopCell.isEmpty()) {
                        updateValue("UiMask", "Topcell", "Barcode = '" + barcode + "'", currentTopCell);
                    }

                    // Update layer in table UiMask
                    QString layer = selectValue("UiMask", "Layer", "UiTzId='" + tzId + "' AND Barcode ='" + barcode + "'").toString();
                    QString currentLayer;
                    programm = "calibredrv -a layout peek " + currentFileName + " -layers";
                    p1.start(programm);
                    if (p1.waitForStarted()) {
                        if (p1.waitForFinished(-1)) {
                            if (p1.exitCode() == 0) {
                                QString output = p1.readAllStandardOutput();
                                currentLayer = output.split(" ")[0].replace("{", "");
                            }
                        }
                    }
                    if (layer.isEmpty() && !currentLayer.isEmpty()) {
                        updateValue("UiMask", "Layer", "Barcode = '" + barcode + "'", currentLayer);
                    }
                    _resultsData << data;
                }
            }
        } else {
            QProcess p;
            QString programm = "svn cat " + orderPath ;
            p.start(programm);
            if (p.waitForStarted()) {
                if (p.waitForFinished(-1)) {
                    if (p.exitCode() == 0) {
                        QString orderContent = p.readAllStandardOutput();
                        foreach (QString group, Functions::getConfigGroups(orderContent)) {
                            if (QRegExp("Mask_\\d+").exactMatch(group)) {
                                QString layer1 = Functions::getConfigValue(orderContent, group + "/layer1").toString();
                                QString layer2 = Functions::getConfigValue(orderContent, group + "/layer2").toString();
                                QString barcode = Functions::getConfigValue(orderContent, group + "/barcode").toString();
                                QString psm = Functions::getConfigValue(orderContent, group + "/psm").toString();

                                QString currentFileName;
                                QString psmFileName;
                                if (lrType == "MLR") {
                                    currentFileName = mpw + "_" + layer1 + "_" + layer2 + "_" + barcode[9] + barcode[10] + barcode[11] + ".oas.gz";
                                    if (psm == "1") {
                                        psmFileName = mpw + "_" + layer1 + "_" + layer2 + "_" + "PSM" + ".oas.gz";
                                    }
                                } else if (lrType == "SLR") {
                                    currentFileName = mpw + "_" + layer1 + "_" + barcode[9] + barcode[10] + barcode[11] + ".oas.gz";
                                    if (psm == "1") {
                                        psmFileName = mpw + "_" + layer1 + "_" + "PSM" + ".oas.gz";
                                    }
                                }
                                if (_workFiles.contains(psmFileName)) {
                                    QStringList data;
                                    QStringList columns;
                                    QStringList values;

                                    columns << "UiTzId";
                                    values << tzId;
                                    columns << "Layer1";
                                    values << layer1;
                                    if (lrType == "MLR") {
                                        columns << "Layer2";
                                        values << layer2;
                                    }

                                    columns << "FileName";
                                    values << psmFileName;
                                    data << psmFileName;
                                    data << "";
                                    QString psmMd5 = Functions::fileCheckSum(psmFileName, QCryptographicHash::Md5);
                                    data << psmMd5;
                                    if (!psmMd5.isEmpty()) {
                                        columns << "FileHash";
                                        values << psmMd5;
                                    }

                                    QString psmTopCell;
                                    QProcess p1;
                                    QString programm = "calibredrv -a layout peek " + psmFileName + " -topcell";
                                    p1.start(programm);
                                    if (p1.waitForStarted()) {
                                        if (p1.waitForFinished(-1)) {
                                            if (p1.exitCode() == 0) {
                                                QString output = p1.readAllStandardOutput();
                                                psmTopCell = output.split("\n")[0];
                                            }
                                        }
                                    }
                                    if (!psmTopCell.isEmpty()) {
                                        columns << "Topcell";
                                        values << psmTopCell;
                                    }
                                    QString psmLayer;
                                    programm = "calibredrv -a layout peek " + psmFileName + " -layers";
                                    p1.start(programm);
                                    if (p1.waitForStarted()) {
                                        if (p1.waitForFinished(-1)) {
                                            if (p1.exitCode() == 0) {
                                                QString output = p1.readAllStandardOutput();
                                                psmLayer = output.split(" ")[0].replace("{", "");
                                            }
                                        }
                                    }
                                    if (!psmLayer.isEmpty()) {
                                        columns << "Layer";
                                        values << psmLayer;
                                    }
                                    QStringList id = selectValue("UiPsmMask", "Id", "UiTzId='" + tzId + "' AND FileName ='" + psmFileName + "'").toStringList();
                                    if (id.isEmpty()) {
                                        insertValue("UiPsmMask",columns,values);
                                    }

                                    _resultsData << data;
                                }

                                if (_workFiles.contains(currentFileName)) {
                                    QStringList data;
                                    data << currentFileName;
                                    data << barcode;

                                    // Update FileName in table UiMask
                                    QString fileName = selectValue("UiMask", "FileName", "UiTzId='" + tzId + "' AND Barcode ='" + barcode + "'").toString();
                                    if (fileName.isEmpty()) {
                                        updateValue("UiMask", "FileName", "Barcode = '" + barcode + "'", currentFileName);
                                    }

                                    // Update md5 in table UiMask
                                    QString md5 = selectValue("UiMask", "FileHash", "UiTzId='" + tzId + "' AND Barcode ='" + barcode + "'").toString();
                                    QString currentMd5 = Functions::fileCheckSum(currentFileName, QCryptographicHash::Md5);

                                    data << currentMd5;

                                    if (md5.isEmpty() && !currentMd5.isEmpty()) {
                                        updateValue("UiMask", "FileHash", "Barcode = '" + barcode + "'", currentMd5);
                                    }

                                    // Update topCell in table UiMask
                                    QString topCell = selectValue("UiMask", "Topcell", "UiTzId='" + tzId + "' AND Barcode ='" + barcode + "'").toString();
                                    QString currentTopCell;
                                    QProcess p1;
                                    QString programm = "calibredrv -a layout peek " + currentFileName + " -topcell";
                                    p1.start(programm);
                                    if (p1.waitForStarted()) {
                                        if (p1.waitForFinished(-1)) {
                                            if (p1.exitCode() == 0) {
                                                QString output = p1.readAllStandardOutput();
                                                currentTopCell = output.split("\n")[0];
                                            }
                                        }
                                    }
                                    if (topCell.isEmpty() && !currentTopCell.isEmpty()) {
                                        updateValue("UiMask", "Topcell", "Barcode = '" + barcode + "'", currentTopCell);
                                    }

                                    // Update layer in table UiMask
                                    QString layer = selectValue("UiMask", "Layer", "UiTzId='" + tzId + "' AND Barcode ='" + barcode + "'").toString();
                                    QString currentLayer;
                                    programm = "calibredrv -a layout peek " + currentFileName + " -layers";
                                    p1.start(programm);
                                    if (p1.waitForStarted()) {
                                        if (p1.waitForFinished(-1)) {
                                            if (p1.exitCode() == 0) {
                                                QString output = p1.readAllStandardOutput();
                                                currentLayer = output.split(" ")[0].replace("{", "");
                                            }
                                        }
                                    }
                                    if (layer == "0" && !currentLayer.isEmpty()) {
                                        updateValue("UiMask", "Layer", "Barcode = '" + barcode + "'", currentLayer);
                                    }
                                    _resultsData << data;
                                }
                            }
                        }
                    }
                }
            }
        }
        database.close();
    } else {
        emit errorMsg("Не удалось подключиться к базе данных (Host:" + commonConfig.value("Sql/host").toString() + " Database: " + commonConfig.value("Sql/database").toString() + ")");
    }
}

bool Flow::check()
{
    QMessageBox question(QMessageBox::Question, "Вы уверены?",
                     "Архив " + _mpw + "_" + _startTime + ".tar.gpg" + "\nбудет загружен на сервер.");
    question.setWindowIcon(QIcon(":/icons/main"));
    QPushButton * confirm = question.addButton("Продолжить", QMessageBox::AcceptRole);
    question.addButton("Отмена", QMessageBox::ResetRole);
    if (_mode == 2) {
        sendTestMail();
        question.setText(question.text() + "\nВы можете проверить письмо, прежде чем файлы будут загружены.");
    }
    question.exec();
    if (question.clickedButton() == confirm)
        return true;
    return false;
}

void Flow::placeholderFiller(QString &str)
{
    str.replace(QRegExp("%MPW%"), _mpw);
    str.replace(QRegExp("%STARTTIME%"), _startTime);
    str.replace(QRegExp("%yy%"), QDateTime::currentDateTime().toString("yy"));
    str.replace(QRegExp("%yyyy%"), QDateTime::currentDateTime().toString("yyyy"));
    str.replace(QRegExp("%MM%"), QDateTime::currentDateTime().toString("MM"));
    str.replace(QRegExp("%d%"), QDateTime::currentDateTime().toString("d"));
    str.replace(QRegExp("%dd%"), QDateTime::currentDateTime().toString("dd"));
    str.replace(QRegExp("%hh%"), QDateTime::currentDateTime().toString("hh"));
    str.replace(QRegExp("%h%"), QDateTime::currentDateTime().toString("h"));
    str.replace(QRegExp("%mm%"), QDateTime::currentDateTime().toString("mm"));
    str.replace(QRegExp("%m%"), QDateTime::currentDateTime().toString("m"));
    str.replace(QRegExp("%ss%"), QDateTime::currentDateTime().toString("ss"));
    str.replace(QRegExp("%s%"), QDateTime::currentDateTime().toString("s"));
}

// Upload files
void Flow::ftpSend()
{
    QString user;
    QString pass;
    QString host;
    int port;
    QString archiveDir;
    QString olDir;
    QString cdDir;
    QString regDir;

    QSettings config(QApplication::instance()->property("currentConfig").toString(), QSettings::IniFormat);
    foreach(QString recipientGroup, config.childGroups().filter(QRegExp("Maskshop_\\d+")))
    {
        config.beginGroup(recipientGroup);
        if (config.value("name").toString() == _maskShop)
        {
            host = config.value("host").toString();
            port = config.value("port").toInt();
            user = config.value("user").toString();
            pass = config.value("pass").toString();
            archiveDir = config.value("archiveDir").toString();
            olDir = config.value("olDir").toString();
            cdDir = config.value("cdDir").toString();
            regDir = config.value("regDir").toString();
        }
        config.endGroup();
    }
    placeholderFiller(archiveDir);
    placeholderFiller(olDir);
    placeholderFiller(cdDir);
    placeholderFiller(regDir);

    QString applicationDirPath(QFileInfo(QApplication::instance()->applicationDirPath()).dir().path());

    connect(&_ps, SIGNAL(readyReadStandardOutput()), this, SLOT(onProcessReadyReadOutput()));
    connect(&_ps, SIGNAL(readyReadStandardError()), this, SLOT(onProcessReadyReadError()));
    connect(&_ps, SIGNAL(finished(int)), this, SLOT(onProcessFinished()));

    foreach (QString path, QStringList() << archiveDir << olDir << cdDir << regDir) {
        if (!path.isEmpty()) {
            _commands.enqueue(applicationDirPath + "/scr/ftpmkd.py " + host + " " + QString::number(port) + " " + user + " " + pass + " " + path);
        }
    }
    if (!_encryptedFiles.isEmpty())
        foreach (QString file, _encryptedFiles)
            _commands.enqueue(applicationDirPath + "/scr/ftpput.py " + host + " " + QString::number(port) + " " + user + " " + pass + " " + file + " " + archiveDir);
    else {
        _error = true;
        emit errorMsg("Ошибка: Не удалось найти архив для загрузки на сервер");
    }

    if (!olDir.isEmpty()) {
        if (_otherFiles.indexOf(QRegExp(".*\\/\\w+OL\\.\\w+")) != -1)
            _commands.enqueue(applicationDirPath + "/scr/ftpput.py " + host + " " + QString::number(port) + " " + user + " " + pass + " " +
                              _otherFiles.at(_otherFiles.indexOf(QRegExp(".*\\/\\w+OL\\.\\w+"))) + " " + olDir);
        else {
            _error = true;
            emit errorMsg("Ошибка: Не удалось найти OrderListing");
        }
    }
    if (!cdDir.isEmpty()) {
        if (_otherFiles.indexOf(QRegExp(".*\\/\\w+CD\\.\\w+")) != -1)
            _commands.enqueue(applicationDirPath + "/scr/ftpput.py " + host + " " + QString::number(port) + " " + user + " " + pass + " " +
                              _otherFiles.at(_otherFiles.indexOf(QRegExp(".*\\/\\w+CD\\.\\w+"))) + " " + cdDir);
        else {
            _error = true;
            emit errorMsg("Ошибка: Не удалось найти файл CD");
        }
    }
    if (!regDir.isEmpty()) {
        if (_otherFiles.indexOf(QRegExp(".*\\/\\w+REG\\.\\w+")) != -1)
            _commands.enqueue(applicationDirPath + "/scr/ftpput.py " + host + " " + QString::number(port) + " " + user + " " + pass + " " +
                              _otherFiles.at(_otherFiles.indexOf(QRegExp(".*\\/\\w+REG\\.\\w+"))) + " " + regDir);
        else {
            _error = true;
            emit errorMsg("Ошибка: Не удалось найти файл REG");
        }
    }

    if (!_error)
        _ps.start(_commands.dequeue());
    else
        emit endRun();
}

void Flow::onProcessReadyReadOutput()
{
    int percent = _ps.readAllStandardOutput().split('\n').first().toInt();
    emit updateProgress(50 + (percent / (_encryptedFiles.length() + _otherFiles.length()) + 100 - (_commands.length() + 1) * 100 / (_encryptedFiles.length() + _otherFiles.length())) / 2);
}

void Flow::onProcessReadyReadError()
{
    QString msg = _ps.readAllStandardError();
    emit errorMsg(msg);
    _log.append(msg);
}

void Flow::onProcessFinished()
{
    if(_ps.exitCode() == 0) {
        if(_ps.program().contains("ftpmkd.py")) {
            emit infoMsg("Создание директории " + _ps.arguments().at(4) + " успешно");
            _log.append(QTime::currentTime().toString("hh:mm:ss") + ": Создание директории " + _ps.arguments().at(4) + " успешно\n");
        }
        if(_ps.program().contains("ftpput.py")) {
            emit infoMsg("Загрузка файла " + _ps.arguments().at(4).section("/", -1, -1) + " завершена");
            _log.append(QTime::currentTime().toString("hh:mm:ss") + ": Загрузка файла " + _ps.arguments().at(4).section("/", -1, -1) + " завершена\n");
        }
    }
    else {
        _error = true;
    }

    if (!_commands.isEmpty() && !_error) {
        _ps.start(_commands.dequeue());
    }
    else {
        emit infoMsg("Загрузка завершена");
        if (_mode == 2) sendMail();
        emit updateProgress(100);
        emit endRun();
    }
}

void Flow::sendMail()
{
    OpfshUtils::Notifier * notifier = new OpfshUtils::Notifier();
    QSettings config(QApplication::instance()->property("currentConfig").toString(), QSettings::IniFormat);
    QString recipients;
    QString subject;
    QString letter;
    QString username = config.value("mail_username").toString();
    QString password = config.value("mail_password").toString();
    QString support = config.value("support").toString();

    foreach(QString recipientGroup, config.childGroups().filter(QRegExp("Maskshop_\\d+"))) {
        config.beginGroup(recipientGroup);
        if (config.value("name").toString() == _maskShop) {
            recipients = config.value("recipients").toString();
            subject = config.value("subject").toString();
            letter = config.value("letter").toString();
        }
        config.endGroup();
    }
    placeholderFiller(subject);
    placeholderFiller(letter);

    if (!_error) {
        notifier->notify("opfshdeamon","(-8E  :-D fun2y pa2sw0rd N2","smtp://mikron-msg-01.mikron.sitronics.com:587","opfshdeamon@niime.ru", QStringList(username), "Загрузка файлов на ftp-сервер завершена успешно", _log, _otherFiles);
        notifier->notify(username.section("@",0,0), password,"smtp://mail.niime.ru:587", username, QStringList(recipients), subject, letter, _otherFiles);
    }
    else {
        notifier->notify("opfshdeamon","(-8E  :-D fun2y pa2sw0rd N2","smtp://mikron-msg-01.mikron.sitronics.com:587","opfshdeamon@niime.ru", QStringList(username), "Загрузка файлов на ftp-сервер завершена с ошибкой", "При загрузке файлов возникла непредвиденная ошибка. Сообщение передано в поддержку");
        notifier->notify("opfshdeamon","(-8E  :-D fun2y pa2sw0rd N2","smtp://mikron-msg-01.mikron.sitronics.com:587","opfshdeamon@niime.ru", QStringList(support), "Загрузка файлов на ftp-сервер завершена с ошибкой", _log);
    }
}

void Flow::sendTestMail()
{
    OpfshUtils::Notifier * notifier = new OpfshUtils::Notifier();
    QSettings config(QApplication::instance()->property("currentConfig").toString(), QSettings::IniFormat);
    QString recipients;
    QString subject;
    QString letter;
    QString username = config.value("mail_username").toString();
    QString password = config.value("mail_password").toString();

    foreach(QString recipientGroup, config.childGroups().filter(QRegExp("Maskshop_\\d+"))) {
        config.beginGroup(recipientGroup);
        if (config.value("name").toString() == _maskShop) {
            recipients = config.value("recipients").toString();
            subject = config.value("subject").toString();
            letter = config.value("letter").toString();
        }
        config.endGroup();
    }
    placeholderFiller(subject);
    placeholderFiller(letter);

    notifier->notify(username.section("@",0,0), password,"smtp://mail.niime.ru:587", username, QStringList(username), "ПРОВЕРКА ПИСЬМА: " + subject, letter, _otherFiles);

}
