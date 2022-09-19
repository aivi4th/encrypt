#include <QCommonStyle>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include "include/browsesvndialog.h"
#include "ui_browsesvndialog.h"


// Constructor 1
BrowseSvnDialog::BrowseSvnDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BrowseSvnDialog)
{
    ui->setupUi(this);
    ui->_openPb->setIcon(QCommonStyle().standardIcon(QStyle::SP_DialogOpenButton));
    ui->_cancelPb->setIcon(QCommonStyle().standardIcon(QStyle::SP_DialogCancelButton));

    QSettings commonConfig(QFileInfo(QApplication::applicationDirPath()).dir().path().append("/cfg/common.conf"), QSettings::IniFormat);
    const QString reticleRoot = commonConfig.value("Svn/reticleRoot").toString();

    _svnModel = new SvnItemModel(reticleRoot, ui->_svnTv);
    ui->_svnTv->setModel(_svnModel);
    ui->_pathLe->setText(_svnModel->rootPath());

    connect(ui->_svnTv, SIGNAL(expanded(QModelIndex)), _svnModel, SLOT(onItemExpanded(QModelIndex)));
    connect(ui->_svnTv->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(onSelectionChanged(QItemSelection,QItemSelection)));
    connect(ui->_svnTv, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onItemDoubleClicked(QModelIndex)));
    connect(ui->_pathLe, SIGNAL(textChanged(QString)), this, SLOT(onPathChanged(QString)));
}


// Destructor
BrowseSvnDialog::~BrowseSvnDialog()
{
    delete ui;
}


// Reimplementation of QDialog::exec() slot
int BrowseSvnDialog::exec()
{
    ui->_svnTv->collapseAll();
    ui->_svnTv->selectionModel()->clearSelection();
    return QDialog::exec();
}


// Do, when file name is changed
void BrowseSvnDialog::onFileNameChanged(const QString &fileName)
{
    if (fileName.trimmed().isEmpty()) {
        ui->_openPb->setEnabled(false);
    } else {
        ui->_openPb->setEnabled(true);
    }
}


// Do, when item of tree view with model index "index" is double clicked
void BrowseSvnDialog::onItemDoubleClicked(const QModelIndex &index)
{
    if (!QRegExp(".+/").exactMatch(index.data().toString())) {
        accept();
    }
}


// Do, when path is changed
void BrowseSvnDialog::onPathChanged(const QString &path)
{
    if (QRegExp(".+/").exactMatch(path)) {
        ui->_openPb->setEnabled(false);
    } else {
        ui->_openPb->setEnabled(true);
    }
}


// Do, when selected item of navigation widget is changed
void BrowseSvnDialog::onSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
    Q_UNUSED(deselected);

    if (!selected.isEmpty()) {
        QModelIndex selectedIndex = selected.indexes()[0];
        ui->_pathLe->setText(_svnModel->filePath(selectedIndex));
    } else {
        ui->_pathLe->setText(_svnModel->rootPath());
    }
}


// Returns selected path
QString BrowseSvnDialog::path() const
{
    return ui->_pathLe->text().trimmed();
}


// Reimplementation of QDialog::show() slot
void BrowseSvnDialog::show()
{
    ui->_svnTv->collapseAll();
    ui->_svnTv->selectionModel()->clearSelection();
    QDialog::show();
}
