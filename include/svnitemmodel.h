#ifndef SVNITEMMODEL_H
#define SVNITEMMODEL_H

#include <QFutureWatcher>
#include <QMap>
#include <QStandardItemModel>

class SvnItemModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit SvnItemModel(QObject *parent = 0);
    explicit SvnItemModel(const QString &rootPath, QObject *parent = 0);
    virtual ~SvnItemModel();
    
signals:
    
public slots:
    QString filePath(const QModelIndex &index);
    void initialize();
    void onItemExpanded(const QModelIndex &index);
    QString rootPath();
    void setRootPath(const QString &rootPath);

private:
    QString _rootPath;
    QMap<QFutureWatcher<QStringList>*, QModelIndex> _watcherMap;

private slots:
    QStringList getChildren(const QModelIndex &index = QModelIndex());
    void initChildrenUpdate(const QModelIndex &index = QModelIndex());
    void updateChildrenSync(const QModelIndex &index = QModelIndex());
    void updateChildrenAsync();
};

#endif // SVNITEMMODEL_H
