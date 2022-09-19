#include <QApplication>
#include "include/appinfodialog.h"
#include "ui_appinfodialog.h"



// Constructor 1
AppInfoDialog::AppInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AppInfoDialog)
{
    ui->setupUi(this);
    ui->_closePb->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));

    ui->_organizationLe->setText(QApplication::organizationName());
    ui->_domainLe->setText(QApplication::organizationDomain());
    ui->_nameLe->setText(QApplication::applicationName());
    ui->_versionLe->setText(QApplication::applicationVersion());
    ui->_descriptionTe->setText(QApplication::instance()->property("description").toString());

}



// Destructor
AppInfoDialog::~AppInfoDialog()
{
    delete ui;
}
