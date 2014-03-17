#include <QtCore/QSettings>
#include <QtCore/QTimer>

#include "amainpoolhandler.h"

// ========================================================================== //
// Конструктор.
// ========================================================================== //
AMainPoolHandler::AMainPoolHandler(QObject *parent)
    : QObject(parent), _timer(new QTimer(this)), _checking_timeout(5) {

    _timer->setSingleShot(false);
    connect(_timer, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));
}


// ========================================================================== //
// Функция возврата наименования файла конфигурации.
// ========================================================================== //
QString AMainPoolHandler::fileName() const {return _fname;}


// ========================================================================== //
// Функция установки файла конфигурации по его наименованию.
// ========================================================================== //
void AMainPoolHandler::setFileName(const QString &fname) {_fname = fname;}


// ========================================================================== //
// Слот активации контроллера.
// ========================================================================== //
void AMainPoolHandler::start() {onTimerTimeout(); _timer->start();}


// ========================================================================== //
// Слот деактивации контроллера.
// ========================================================================== //
void AMainPoolHandler::stop() {_timer->stop();}


// ========================================================================== //
// Слот активации таймера.
// ========================================================================== //
void AMainPoolHandler::onTimerTimeout() {
    int checking_interval = 5, checking_timeout = 5;

    QString pool("localhost:3336");

    QSettings settings(_fname, QSettings::IniFormat);

    settings.beginGroup("MainPool");

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

    if(settings.contains("checking-timeout")) {
        const int interval = settings.value("checking-timeout").toInt();
        if(interval > 0) checking_timeout = interval;
        else settings.remove("checking-timeout");

    } else settings.setValue("checking-timeout", checking_timeout);

    if(_checking_timeout != checking_timeout) {
        _checking_timeout = checking_timeout;
        emit checkingIntervalChanged(checking_timeout);
    }

    if(settings.contains("stratum")) {
        const QString p = settings.value("stratum").toString();
        if(!p.isEmpty()) pool = p; else settings.remove("stratum");

    } else settings.setValue("stratum", pool);

    if(_pool != pool) {_pool = pool; emit poolChanged(pool);}

    settings.endGroup();
}
