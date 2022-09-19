#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCheckBox>
#include <QFileSystemModel>
#include <QItemSelection>
#include <QMainWindow>
#include <QProcess>
#include "settingsdialog.h"
#include "browsesvndialog.h"
#include "flow.h"
#include "outputdialog.h"
#include "appinfodialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private:
    QList<QCheckBox*> _fileCbs;
    Flow* _flow;
    AppInfoDialog* _appInfoDialog;
    OutputDialog* _outputDialog;
    BrowseSvnDialog* _browseSvnDialog;
    QStringList _filePaths;
    Ui::MainWindow *ui;
    QString _filePath;

private slots:
    void addRecipient(const QString &name = QString());
    bool isConfigurationValid();
    void launch();
    void launchDoc();
    void onOrderPathChanged(const QString filePath);
    void onSelectionChanged();
    void onBrowsePbClicked();
    void onSvnPbClicked();
    void removeRecipient();
    void resizeEvent(QResizeEvent *event);
    void showFiles();
    void showSettingsDialog();
    void toggleFiles();
    void updateFilesCb();
    void updateHelpMenu();
    void on__ftpCb_stateChanged(int arg1);
};

#endif // MAINWINDOW_H
