#ifndef FLOW_H
#define FLOW_H

#include <QObject>
#include <QStringList>
#include <QVariant>
#include <QtConcurrentRun>
#include <QFutureWatcher>
#include <QProcess>
#include <QQueue>

class Flow : public QObject
{
    Q_OBJECT

public:
    explicit Flow(const QString &orderPath, const QString &maskShop, const QStringList &recipients, const QStringList &files, int mode = 0, bool check = false, QObject *parent = 0);
    ~Flow();

signals:
    void updateProgress(const int &value);
    void infoMsg(const QString &string);
    void errorMsg(const QString &string);
    void endEncriptyon();
    void endRun();

private:
    QStringList _workFiles;
    QStringList _encryptedFiles;
    QStringList _otherFiles;
    QString _startDir;
    QString _workDir;
    QString _startTime;
    QFuture<void> _actionsFuture;
    QFutureWatcher<void> _actionsWatcher;
    QList<QStringList> _resultsData;
    QProcess _ps;
    QQueue<QString> _commands;
    int _mode;
    bool _check;
    QString _maskShop;
    bool _error;
    QString _mpw;
    QString _log;

private slots:
    QString archivingFiles();
    void catOrderFile(const QString &orderPath);
    void actions(const QString &orderPath, const QString &maskShop, const QStringList &recipients, const QStringList &files);
    QStringList encriptyonFiles(const QStringList &files, const QStringList &recipients);
    void placeholderFiller(QString &str);
    bool check();
    void ftpSend();
    void sendMail();
    void sendTestMail();
    void loadingFiles(const QStringList &files);
    void loadOtherFiles(const QStringList &files);
    QVariant selectValue(const QString &table, const QString &column, const QString &condition);
    void updateValue(const QString &table, const QString &column, const QString &condition, const QString &value);
    void insertValue(const QString &table, const QStringList &columns, const QStringList &values);
    void initActions(const QString &orderPath, const QString &maskShop, const QStringList &recipients, const QStringList &files);
    void postActions(const QString &orderPath);
    void onProcessReadyReadOutput();
    void onProcessReadyReadError();
    void onProcessFinished();
};

#endif // FLOW_H
