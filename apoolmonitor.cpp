#include <QtCore/QTimer>

#include <QtNetwork/QTcpSocket>

#include "apoolmonitor.h"
#include "alogger.h"

// ========================================================================== //
// Конструктор.
// ========================================================================== //
APoolMonitor::APoolMonitor(QObject *parent)
    : QObject(parent), _state(STATE_RDY_WRITE), _active(false)
    , _host("localhost"), _port(3337), _timer(new QTimer(this))
    , _socket(new QTcpSocket(this)) {

    _timer->setInterval(5000);
    _timer->setSingleShot(true);

    connect(_timer, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));

    connect(_socket, SIGNAL(readyRead()), this, SLOT(onSocketReadyRead()));
    connect(_socket, SIGNAL(error(QAbstractSocket::SocketError))
        , this, SLOT(onSocketError()));
}


// ========================================================================== //
// Функция установки хоста.
// ========================================================================== //
void APoolMonitor::setHost(const QString &host) {
    if(!host.isEmpty()) {
        const bool active = _active;
        if(active) stop();

        _host = host;

        if(active) start();
    }
}


// ========================================================================== //
// Функция установки хоста.
// ========================================================================== //
void APoolMonitor::setHost(const QString &host, int port) {
    if(!host.isEmpty() && port > 0) {
        const bool active = _active;
        if(active) stop();

        _host = host; _port = port;

        if(active) start();
    }
}


// ========================================================================== //
// Функция установки порта.
// ========================================================================== //
void APoolMonitor::setPort(int port) {
    if(port > 0) {
        const bool active = _active;
        if(active) stop();

        _port = port;

        if(active) start();
    }
}


// ========================================================================== //
// Слот активации мониторинга.
// ========================================================================== //
void APoolMonitor::start() {
    _state = STATE_RDY_WRITE; _socket->connectToHost(_host, _port);

    _timer->start(); _active = true;
}


// ========================================================================== //
// Слот деактивации мониторинга.
// ========================================================================== //
void APoolMonitor::stop() {_timer->stop(); _socket->abort(); _active = false;}


// ========================================================================== //
// Слот активации таймера.
// ========================================================================== //
void APoolMonitor::onTimerTimeout() {
    if(_socket->state() != QAbstractSocket::ConnectedState) {
        if(_socket->state() == QAbstractSocket::UnconnectedState) {
            _socket->connectToHost(_host, _port); _timer->start();
        }

        return;
    }

    switch(_state) {
        case STATE_RDY_WRITE: {
            QByteArray data;
            data.append("{");
            data.append("\"id\": 1");
            data.append(", \"method\": \"mining.subscribe\"");
            data.append(", \"params\": []");
            data.append("}\n");

            _socket->write(data);

            _state = STATE_RDY_READ;
        } break;

        case STATE_RDY_READ: {
            logWarn("stratum is not alive");

            _state = STATE_RDY_WRITE; stop(); emit failed();
        } break;
    }
}


// ========================================================================== //
// Слот приёма сетевых сообщений.
// ========================================================================== //
void APoolMonitor::onSocketReadyRead() {
    logInfo("stratum is alive"); _socket->abort(); emit succeed();
}


// ========================================================================== //
// Слот обработки ошибок сетевой передачи данных.
// ========================================================================== //
void APoolMonitor::onSocketError() {
    logWarn(_socket->errorString()); stop(); emit failed();
}
