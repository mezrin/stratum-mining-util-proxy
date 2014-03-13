#include <QtCore/QCoreApplication>
#include <QtCore/QSettings>
#include <QtCore/QFile>

#include "aconfighandler.h"
#include "alogger.h"

// ========================================================================== //
// Конструктор.
// ========================================================================== //
AConfigHandler::AConfigHandler(QObject *parent) : QObject(parent) {}


// ========================================================================== //
// Функция возврата наименования файла конфигурации.
// ========================================================================== //
QString AConfigHandler::fileName() const {return _fname;}


// ========================================================================== //
// Функция установки файла конфигурации по его наименованию.
// ========================================================================== //
void AConfigHandler::setFileName(const QString &fname) {
    if(!QFile::exists(fname)) {
        QSettings settings(fname, QSettings::IniFormat);

        if(qApp->property("server-port").isValid())
            settings.setValue("server-port", qApp->property("server-port"));
        else {
            qApp->setProperty("server-port", 3400);
            settings.setValue("server-port", 3400);
        }

        settings.beginGroup("BackupPools");
        settings.beginWriteArray("Pools");
        settings.setArrayIndex(0);
        settings.setValue("stratum", "localhost:3336");
        settings.endArray();
        settings.endGroup();

    } else {
        if(!qApp->property("server-port").isValid()) {
            QSettings settings(fname, QSettings::IniFormat);
            if(settings.value("server-port").isValid()) {
                qApp->setProperty("server-port", settings.value("server-port"));

            } else {
                qApp->setProperty("server-port", 3400);
                settings.setValue("server-port", 3400);
            }
        }
    }

    _fname = fname;
}
