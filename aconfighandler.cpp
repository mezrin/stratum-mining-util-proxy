#include <QtCore/QCoreApplication>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtCore/QFile>

#include "aconfighandler.h"
#include "alogger.h"

// ========================================================================== //
// Конструктор.
// ========================================================================== //
AConfigHandler::AConfigHandler(QObject *parent)
    : QObject(parent), _timer(new QTimer(this)), _pool_index(0) {

    _timer->setSingleShot(false);
    connect(_timer, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));
}


// ========================================================================== //
// Функция возврата наименования файла конфигурации.
// ========================================================================== //
QString AConfigHandler::fileName() const {return _fname;}


// ========================================================================== //
// Функция установки файла конфигурации по его наименованию.
// ========================================================================== //
void AConfigHandler::setFileName(const QString &fname) {
    if(fname.isEmpty()) {logCrit("name of config file is empty"); return;}

    reset();

    if(!QFile::exists(fname)) {
        QSettings settings(fname, QSettings::IniFormat);

        if(qApp->property("server-port").isValid())
            settings.setValue("server-port", qApp->property("server-port"));
        else {
            qApp->setProperty("server-port", 3400);
            settings.setValue("server-port", 3400);
        }

        if(qApp->property("config-interval").isValid()) {
            settings.setValue("config-interval"
                , qApp->property("config-interval"));

        } else {
            qApp->setProperty("config-interval", 60);
            settings.setValue("config-interval", 60);
        }

        if(qApp->property("pool-interval").isValid())
            settings.setValue("pool-interval", qApp->property("pool-interval"));
        else {
            qApp->setProperty("pool-interval", 5);
            settings.setValue("pool-interval", 5);
        }

        settings.beginWriteArray("Pools");
        settings.setArrayIndex(0);
        settings.setValue("stratum", "localhost:3336");
        settings.endArray();

    } else {
        const bool server_port = qApp->property("server-port").isValid();

        const bool config_interval
            = qApp->property("config-interval").isValid();

        const bool pool_interval = qApp->property("pool-interval").isValid();

        if(!(server_port && config_interval && pool_interval)) {
            QSettings settings(fname, QSettings::IniFormat);

            if(!server_port) {
                if(settings.value("server-port").isValid()) {
                    qApp->setProperty("server-port"
                        , settings.value("server-port"));

                } else qApp->setProperty("server-port", 3400);
            }

            if(!config_interval) {
                if(settings.value("config-interval").isValid()) {
                    qApp->setProperty("config-interval"
                        , settings.value("config-interval"));

                } else qApp->setProperty("config-interval", 60);
            }

            if(!pool_interval) {
                if(settings.value("pool-interval").isValid()) {
                    qApp->setProperty("pool-interval"
                        , settings.value("pool-interval"));

                } else qApp->setProperty("pool-interval", 5);
            }
        }
    }

    _timer->setInterval(qApp->property("config-interval").toInt()*1000);

    _fname = fname;
}


// ========================================================================== //
// Функция возврата текущего пула.
// ========================================================================== //
QString AConfigHandler::pool() const {return _pool;}


// ========================================================================== //
// Слот активации мониторинга файла конфигурации.
// ========================================================================== //
void AConfigHandler::start() {reset(); onTimerTimeout(); _timer->start();}


// ========================================================================== //
// Слот деактивации мониторинга файла конфигурации.
// ========================================================================== //
void AConfigHandler::stop() {_timer->stop(); reset();}


// ========================================================================== //
// Слот переключения текущего пула.
// ========================================================================== //
void AConfigHandler::nextPool() {++_pool_index;}


// ========================================================================== //
// Функция сброса значений параметров.
// ========================================================================== //
void AConfigHandler::reset() {_pool_index = 0; _pool.clear();}


// ========================================================================== //
// Слот активации таймера.
// ========================================================================== //
void AConfigHandler::onTimerTimeout() {
    QSettings settings(_fname, QSettings::IniFormat);

    const int pools = settings.beginReadArray("Pools");
    if(_pool_index > 0 && _pool_index >= pools) reset();

    do {
        settings.setArrayIndex(_pool_index);

        QVariant pool_var = settings.value("stratum");
        if(pool_var.isValid()) {
            QString pool = pool_var.toString();
            if(!pool.isEmpty()) {
                if(_pool != pool) {_pool = pool; emit poolChanged(_pool);}

                break;
            }
        }

    } while(++_pool_index < pools);

    settings.endArray();
}
