#include <QApplication>
#include <QDesktopWidget>
#include <QFileInfo>
#include <QTextCodec>
#include "include/mainwindow.h"

int main(int argc, char *argv[])
{
    //QTextCodec::setCodecForCStrings(QTextCodec::codecForLocale());
    //QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));

    QApplication app(argc, argv);
    app.setOrganizationName(QObject::tr("АО \"НИИМЭ\""));
    app.setOrganizationDomain("niime.ru");
    app.setApplicationName(QFileInfo(app.applicationFilePath()).fileName());
    app.setApplicationVersion(QString("%1.%2.%3_hf1").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_BUILD));
    app.setProperty("description", QVariant("Программа для шифрования масок для передачи на фабрику изготовления фотошаблонов"));

    app.setProperty("defaultConfig", QFileInfo(app.applicationDirPath()).dir().path().append("/cfg/default.conf"));
    app.setProperty("userConfig", QDir::homePath().append("/.").append(app.applicationName()).append("/user.conf"));
    app.setProperty("currentConfig", app.property("userConfig").toString());
    app.setProperty("keyOpfsh", true);
    QFile(app.property("userConfig").toString()).setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);

    MainWindow mainWindow;
    mainWindow.setWindowTitle(app.applicationName().append(" "));
    int x = QApplication::desktop()->availableGeometry(QCursor::pos()).center().x();
    int y = QApplication::desktop()->availableGeometry(QCursor::pos()).center().y();
    x-= mainWindow.width()/2;
    y-= mainWindow.height()/2;
    mainWindow.move(QPoint(x,y));
    mainWindow.show();

    return app.exec();
}
