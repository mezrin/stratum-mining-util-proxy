#include <QtCore/QTimer>

#include <QtNetwork/QTcpSocket>

#include "apoolchecker.h"
#include "alogger.h"

// ========================================================================== //
// Конструктор.
// ========================================================================== //
APoolChecker::APoolChecker(QObject *parent)
    : QObject(parent), _socket(new QTcpSocket(this)), _timer(new QTimer(this)) {

    connect(_socket, SIGNAL(connected()), this, SLOT(onSocketConnected()));
    connect(_socket, SIGNAL(readyRead()), this, SLOT(onSocketReadyRead()));
    connect(_socket, SIGNAL(error(QAbstractSocket::SocketError))
        , this, SLOT(onSocketError()));

    _timer->setInterval(5000);
    _timer->setSingleShot(true);

    connect(_timer, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));
}


// ========================================================================== //
// Слот проверки активности пула.
// ========================================================================== //
void APoolChecker::check(const QString &host, int port) {
    if(!host.isEmpty() && port > 0) {
        if(_socket->state() != QAbstractSocket::UnconnectedState)
            _socket->abort();

        return _socket->connectToHost(host, port);
    }

    emit failed();
}


// ========================================================================== //
// Слот подключения сокета к хосту.
// ========================================================================== //
void APoolChecker::onSocketConnected() {
    QByteArray data;
    data.append("{");
    data.append("\"id\": 1");
    data.append(", \"method\": \"mining.subscribe\"");
    data.append(", \"params\": []");
    data.append("}\n");

    _socket->write(data); _timer->start();
}


// ========================================================================== //
// Слот приёма сетевых сообщений.
// ========================================================================== //
void APoolChecker::onSocketReadyRead() {
    _timer->stop(); _socket->disconnectFromHost(); emit succeed();
}


// ========================================================================== //
// Слот обработки ошибок сетевой передачи данных.
// ========================================================================== //
void APoolChecker::onSocketError() {
    _timer->stop(); _socket->abort();

    logWarn(_socket->errorString()); emit failed();
}


// ========================================================================== //
// Слот активации таймера.
// ========================================================================== //
void APoolChecker::onTimerTimeout() {
    logWarn(QString("pool %1:%2 freeze")
        .arg(_socket->peerName()).arg(_socket->peerPort()));

    _socket->abort(); emit failed();
}
