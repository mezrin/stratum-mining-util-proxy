#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>
#include <QtCore/QDateTime>
#include <QtCore/QFile>

#include "alogger.h"

// ========================================================================== //
// Функция установки сообщения.
// ========================================================================== //
void ALogger::info(const char *msg) {
    QString txt
        = QString("[%1] %2: %3")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
            .arg("INFO").arg(msg);

    emit sigInfo(txt); save(txt);
}


// ========================================================================== //
// Функция установки предупреждения.
// ========================================================================== //
void ALogger::warn(const char *msg) {
    QString txt
        = QString("[%1] %2: %3")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
            .arg("WARN").arg(msg);

    emit sigWarn(txt); save(txt);
}


// ========================================================================== //
// Функция установки ошибки.
// ========================================================================== //
void ALogger::crit(const char *msg) {
    QString txt
        = QString("[%1] %2: %3")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
            .arg("CRIT").arg(msg);

    emit sigCrit(txt); save(txt);
}


// ========================================================================== //
// Конструктор.
// ========================================================================== //
ALogger::ALogger(QObject *parent) : QObject(parent) {
    _fname
        = QCoreApplication::applicationDirPath()
            + QCoreApplication::applicationName() + ".log";
}


// ========================================================================== //
// Функция установки файла логирования.
// ========================================================================== //
void ALogger::setFileName(const QString &fname) {_fname = fname;}


// ========================================================================== //
// Функция сохранения сообщения в файл.
// ========================================================================== //
void ALogger::save(const QString &msg) {
    QFile file(_fname);
    if(file.open(QFile::WriteOnly|QFile::Append|QFile::Text)) {
        QTextStream stream(&file);
        stream << msg << endl;
        file.close();
    }
}
