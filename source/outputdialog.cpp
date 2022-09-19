#include "include/outputdialog.h"
#include "ui_outputdialog.h"
#include <QApplication>

OutputDialog::OutputDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OutputDialog)
{
    ui->setupUi(this);
    this->setWindowTitle(QApplication::applicationName() + ": Выполнение");
    this->show();
}

OutputDialog::~OutputDialog()
{
    delete ui;
}

// Append info message in text edit box
void OutputDialog::infoMessage(const QString &string)
{
    ui->_outputTe->append(string);
}

// Append error message in text edit box
void OutputDialog::errorMessage(const QString &string)
{
    ui->_outputTe->append("<font color=red>" + string + "</font>");
}

// Set progress value in _outputProgressPrb
void OutputDialog::setProgressValue(const int &value)
{
    ui->_outputProgressPrb->setValue(value);
}
