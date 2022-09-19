#ifndef BROWSESVNDIALOG_H
#define BROWSESVNDIALOG_H

#include <QDialog>
#include <QItemSelection>
#include "svnitemmodel.h"

namespace Ui {
class BrowseSvnDialog;
}

class BrowseSvnDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit BrowseSvnDialog(QWidget *parent = 0);
    ~BrowseSvnDialog();

public slots:
    int exec();
    QString path() const;
    void show();
    
private:
    SvnItemModel* _svnModel;
    Ui::BrowseSvnDialog *ui;

private slots:
    void onFileNameChanged(const QString &fileName);
    void onItemDoubleClicked(const QModelIndex &index);
    void onPathChanged(const QString &path);
    void onSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
};

#endif // BROWSESVNDIALOG_H
