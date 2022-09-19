#include <QDebug>
#include <QWidget>
#include <QFileInfo>
#include <QTextStream>
#include "include/common.h"

// Clears layout
void Functions::clearLayout(QLayout* layout, bool deleteWidgets)
{
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (deleteWidgets) {
            if (QWidget* widget = item->widget()) {
                delete widget;
            }
        }
        if (QLayout* childLayout = item->layout()) {
            clearLayout(childLayout, deleteWidgets);
        }
        delete item;
    }
}

// Returns argument value with key "argKey" from argument list "args"
QVariant Functions::getArgValue(const QStringList args, const QString &arg)
{
    QStringList values;
    for(int i = 0; i < args.size(); i++) {
        if (args[i] == arg) {
            for(int j = i + 1; j < args.size(); j++) {
                if (!QRegExp("^-{1,2}\\S*$").exactMatch(args[j])) {
                    values << args[j];
                } else {
                    break;
                }
            }
        }
    }
    return QVariant(values);
}



// Retrurns config groups from config content string
QStringList Functions::getConfigGroups(const QString &configContent)
{
    QStringList groups;
    foreach (QString line, configContent.split('\n')) {
        if (QRegExp("^\\s*\\[.*\\]\\s*$").exactMatch(line)) {
            line.remove(QRegExp("[\\[\\]]"));
            groups << line.trimmed();
        }
    }
    return groups;
}



// Retrurns config keys from config content string
QStringList Functions::getConfigKeys(const QString &configContent)
{
    QStringList keys;
    QString group;
    foreach (QString line, configContent.split('\n')) {
        if (QRegExp("^\\s*\\[.*\\]\\s*$").exactMatch(line)) {
            line.remove(QRegExp("[\\[\\]]"));
            group = line.trimmed();
        } else if (QRegExp("^\\s*\\S+\\s*=.*$").exactMatch(line)) {
            if (group.isEmpty()) {
                keys << line.split('=')[0].trimmed();
            } else {
                keys << QString(group).append('/').append(line.split('=')[0].trimmed());
            }
        }
    }
    return keys;
}



// Retrurns value of config key from config content string
QVariant Functions::getConfigValue(const QString &configContent, const QString &configKey)
{
    QVariant value;

    QString group;
    QString key;
    if (QRegExp("^\\S+/\\S+$").exactMatch(configKey)) {
        group = configKey.split('/')[0];
        key = configKey.split('/')[1];
    } else {
        key = configKey;
    }

    QString currentGroup;
    foreach (QString line, configContent.split('\n')) {
        if (QRegExp("^\\s*\\[.*\\]\\s*$").exactMatch(line)) {
            line.remove(QRegExp("[\\[\\]]"));
            currentGroup = line.trimmed();
        }

        if ((currentGroup == group || (currentGroup == "General" && group.isEmpty())) &&
            QRegExp(QString("^\\s*").append(key).append("\\s*=\\s*\\S.*$")).exactMatch(line))
        {
            value = line.split('=')[1].trimmed().split(QRegExp("\\s*,\\s*"));
            break;
        }
    }

    return value;
}

// Returns check sum generated using algorithm "hashAlgorithm" for a file with path "filePath"
QByteArray Functions::fileCheckSum(const QString &filePath, QCryptographicHash::Algorithm hashAlgorithm)
{
    QFile f(filePath);
    if (f.open(QFile::ReadOnly)) {
        QCryptographicHash hash(hashAlgorithm);
        hash.addData(f.readAll());
        return hash.result().toHex();
    }
    return QByteArray();
}

