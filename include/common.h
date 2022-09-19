#ifndef COMMON_H
#define COMMON_H

#include <QDebug>
#include <QLayout>
#include <QVariant>
#include <QCryptographicHash>

namespace Functions
{
    void clearLayout(QLayout* layout, bool deleteWidgets);
    QByteArray fileCheckSum(const QString &filePath, QCryptographicHash::Algorithm hashAlgorithm);
    QVariant getArgValue(const QStringList args, const QString &arg);
    QStringList getConfigGroups(const QString &configContent);
    QStringList getConfigKeys(const QString &configContent);
    QVariant getConfigValue(const QString &configContent, const QString &configKey);
}


#endif // COMMON_H
