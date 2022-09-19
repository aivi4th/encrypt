#include <QFileIconProvider>
#include <QProcess>
#include <QtConcurrentRun>
#include "include/svnitemmodel.h"



// Constructor 1
SvnItemModel::SvnItemModel(QObject *parent) :
    QStandardItemModel(parent)
{

}



// Consturctor 2
SvnItemModel::SvnItemModel(const QString &rootPath, QObject *parent) :
    QStandardItemModel(parent),
    _rootPath(rootPath)
{
    initialize();
}



// Destructor
SvnItemModel::~SvnItemModel()
{
    foreach(QFutureWatcher<QStringList>* watcher, _watcherMap.keys()) {
        watcher->cancel();
        watcher->waitForFinished();
    }
}



// Returns children of item with index "index"
QStringList SvnItemModel::getChildren(const QModelIndex &index)
{
    QStringList children;

    QString path;
    if (index.isValid()) {
        path = index.data().toString();
        QModelIndex parent = index.parent();
        while(parent.isValid()) {
            path.prepend(parent.data().toString().append("/"));
            parent = parent.parent();
        }
    }
    path.prepend(QString(_rootPath).append("/"));

    QProcess p;
    p.start(QString("svn list ").append(path));
    if (p.waitForStarted() && p.waitForFinished() && p.exitCode() == 0) {
        foreach(QString entry, QString(p.readAllStandardOutput()).split("\n", QString::SkipEmptyParts)) {
            children << entry;
        }
    }

    return children;
}



// Initiates children update for item with index "index"
void SvnItemModel::initChildrenUpdate(const QModelIndex &index)
{
    QFutureWatcher<QStringList>* watcher = new QFutureWatcher<QStringList>(this);
    connect(watcher, SIGNAL(finished()), this, SLOT(updateChildrenAsync()));
    _watcherMap[watcher] = index;
    watcher->setFuture(QtConcurrent::run(this, &SvnItemModel::getChildren, index));
}



// Returns full path for model index "index"
QString SvnItemModel::filePath(const QModelIndex &index)
{
    QString path;

    if (index.isValid()) {
        path = index.data().toString();
        QModelIndex parent = index.parent();
        while(parent.isValid()) {
            path.prepend(parent.data().toString());
            parent = parent.parent();
        }
        path.prepend(QString(_rootPath) + "/");
    }

    return path;
}



// Initializes model
void SvnItemModel::initialize()
{
    updateChildrenSync();
    QStandardItem* root = this->invisibleRootItem();
    for (int i = 0; i < this->rowCount(); i++) {
        updateChildrenSync(root->child(i, 0)->index());
    }
}



// Do, when item of the widget representing the model with index "index" has been expanded
void SvnItemModel::onItemExpanded(const QModelIndex &index)
{
    QModelIndex child;
    for(int i = 0; i < this->rowCount(index); i++) {
        child = index.child(i, 0);
        QString statusTip = this->itemFromIndex(child)->statusTip();
        if (QRegExp(".+/").exactMatch(child.data().toString()) && statusTip != "updated") {
            initChildrenUpdate(index.child(i, 0));
        }
    }
}



// Returns root path
QString SvnItemModel::rootPath()
{
    return _rootPath;
}



// Sets root path
void SvnItemModel::setRootPath(const QString &rootPath)
{
    _rootPath = rootPath;
}



// Updates item's children with index "index" in synchronous mode
void SvnItemModel::updateChildrenSync(const QModelIndex &index)
{
    QFileIconProvider fileIconProvider;

    QStandardItem* item;
    QIcon icon;
    foreach(QString entry, getChildren(index)) {
        if (QRegExp(".+/").exactMatch(entry)) {
            icon = fileIconProvider.icon(QFileIconProvider::Folder);
        } else {
            icon = fileIconProvider.icon(QFileIconProvider::File);
        }
        item = new QStandardItem(icon, entry);
        if (index.isValid()) {
            this->itemFromIndex(index)->appendRow(item);
        } else {
            this->appendRow(item);
        }
    }

    if (index.isValid()) {
        this->itemFromIndex(index)->setStatusTip("updated");
    }
}




// Updates item's children in asynchronous mode
void SvnItemModel::updateChildrenAsync()
{
    QFutureWatcher<QStringList>* watcher = static_cast<QFutureWatcher<QStringList>* >(sender());
    if (watcher) {

        QFileIconProvider fileIconProvider;

        QModelIndex index = _watcherMap[watcher];
        QStandardItem* item;
        QIcon icon;
        foreach(QString entry, watcher->result()) {
            if (QRegExp(".+/").exactMatch(entry)) {
                icon = fileIconProvider.icon(QFileIconProvider::Folder);
            } else {
                icon = fileIconProvider.icon(QFileIconProvider::File);
            }
            item = new QStandardItem(icon, entry);
            if (index.isValid()) {
                this->itemFromIndex(index)->appendRow(item);
            } else {
                this->appendRow(item);
            }
        }
        if (index.isValid()) {
            this->itemFromIndex(index)->setStatusTip("updated");
        }

        _watcherMap.remove(watcher);
        delete watcher;
    }
}




