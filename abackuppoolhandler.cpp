#include <QtCore/QSettings>
#include <QtCore/QTimer>

#include "abackuppoolhandler.h"

// ========================================================================== //
// Конструктор.
// ========================================================================== //
ABackupPoolHandler::ABackupPoolHandler(QObject *parent)
    : QObject(parent), _timer(new QTimer(this)), _pool_index(0) {

    _timer->setSingleShot(false);
    connect(_timer, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));
}


// ========================================================================== //
// Функция возврата наименования файла конфигурации.
// ========================================================================== //
QString ABackupPoolHandler::fileName() const {return _fname;}


// ========================================================================== //
// Функция установки файла конфигурации по его наименованию.
// ========================================================================== //
void ABackupPoolHandler::setFileName(const QString &fname) {_fname = fname;}


// ========================================================================== //
// Слот активации контроллера.
// ========================================================================== //
void ABackupPoolHandler::start() {
    _pool.clear(); _pool_index = 0; onTimerTimeout(); _timer->start();
}


// ========================================================================== //
// Слот деактивации контроллера.
// ========================================================================== //
void ABackupPoolHandler::stop() {_timer->stop();}


// ========================================================================== //
// Слот переключения текущего пула.
// ========================================================================== //
void ABackupPoolHandler::nextPool() {++_pool_index; onTimerTimeout();}


// ========================================================================== //
// Слот активации таймера.
// ========================================================================== //
void ABackupPoolHandler::onTimerTimeout() {
    int checking_interval = 5;

    QSettings settings(_fname, QSettings::IniFormat);

    settings.beginGroup("BackupPools");

    if(settings.contains("checking-interval")) {
        const int interval = settings.value("checking-interval").toInt();
        if(interval > 0) checking_interval = interval;
        else settings.remove("checking-interval");

    } else settings.setValue("checking-interval", checking_interval);

    const int checking_interval_ms = checking_interval * 1000;
    if(_timer->interval() != checking_interval_ms) {
        _timer->setInterval(checking_interval_ms);
        emit checkingIntervalChanged(checking_interval);
    }

    const int pools = settings.beginReadArray("Pools");
    if(_pool_index > 0 && _pool_index >= pools) {
        _pool.clear(); _pool_index = 0;
    }

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

    settings.endGroup();
}
