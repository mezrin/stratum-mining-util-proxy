#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
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

    emit sigInfo(txt); save(txt); if(_has_terminal_log) qDebug() << txt;
}


// ========================================================================== //
// Функция установки предупреждения.
// ========================================================================== //
void ALogger::warn(const char *msg) {
    QString txt
        = QString("[%1] %2: %3")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
            .arg("WARN").arg(msg);

    emit sigWarn(txt); save(txt); if(_has_terminal_log) qDebug() << txt;
}


// ========================================================================== //
// Функция установки ошибки.
// ========================================================================== //
void ALogger::crit(const char *msg) {
    QString txt
        = QString("[%1] %2: %3")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
            .arg("CRIT").arg(msg);

    emit sigCrit(txt); save(txt); if(_has_terminal_log) qDebug() << txt;
}


// ========================================================================== //
// Функция установки флага вывода сообщений на консоль.
// ========================================================================== //
void ALogger::setHasTerminalLog(bool has_terminal_log) {
    _has_terminal_log = has_terminal_log;
}


// ========================================================================== //
// Функция установки файла логирования.
// ========================================================================== //
void ALogger::setFileName(const QString &fname) {_fname = fname;}


// ========================================================================== //
// Конструктор.
// ========================================================================== //
ALogger::ALogger(QObject *parent) : QObject(parent), _has_terminal_log(false) {
    _fname
        = QCoreApplication::applicationDirPath() + "/"
            + QCoreApplication::applicationName() + ".log";
}


// ========================================================================== //
// Функция сохранения сообщения в файл.
// ========================================================================== //
void ALogger::save(const QString &msg) {
    QFile file(_fname);
    if(file.exists() && file.size() >= (1024*1024*10)) {
        int i = 3;
        while(i) {
            const QString new_fname = _fname + "." + QString::number(i+1);
            if(QFile::exists(new_fname)) {
                QFile f(new_fname); if(!f.remove()) break;
            }

            const QString old_fname = _fname + "." + QString::number(i);
            if(QFile::exists(old_fname)) {
                if(!QFile::rename(old_fname, new_fname)) break;
            }

            --i;
        }

        bool ok = true;

        const QString new_fname = _fname + ".1";
        if(QFile::exists(new_fname)) {
            QFile f(new_fname); if(!f.remove()) ok = false;
        }

        if(ok) QFile::rename(_fname, new_fname);
    }

    if(file.open(QFile::WriteOnly|QFile::Append|QFile::Text)) {
        QTextStream stream(&file);
        stream << msg << endl;
        file.close();
    }
}
