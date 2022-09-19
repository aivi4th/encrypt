#ifndef APPINFODIALOG_H
#define APPINFODIALOG_H

#include <QDialog>

namespace Ui {
class AppInfoDialog;
}

class AppInfoDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit AppInfoDialog(QWidget *parent = 0);
    ~AppInfoDialog();
    
private:
    Ui::AppInfoDialog *ui;
};

#endif // APPINFODIALOG_H
