#ifndef OUTPUTDIALOG_H
#define OUTPUTDIALOG_H

#include <QDialog>

namespace Ui {
    class OutputDialog;
}

class OutputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OutputDialog(QWidget *parent = 0);
    ~OutputDialog();

public slots:
    void infoMessage(const QString &string);
    void errorMessage(const QString &string);
    void setProgressValue(const int &value);

private:
    Ui::OutputDialog *ui;
};

#endif // OUTPUTDIALOG_H
