#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QFile>
#include <QtCore/QDir>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include "aproxymachine.h"
#include "aunixsignalhandler.h"
#include "aconfighandler.h"
#include "apoolmonitor.h"
#include "alogger.h"

// ========================================================================== //
// Конструктор.
// ========================================================================== //
AProxyMachine::AProxyMachine(QObject *parent)
    : QObject(parent), _active(false), _config_handler(new AConfigHandler(this))
    , _pool_monitor(new APoolMonitor(this)), _server(NULL)
    , _work_path(QCoreApplication::applicationDirPath()) {

    connect(_config_handler, SIGNAL(poolChanged(const QString&))
        , _pool_monitor, SLOT(changePool(const QString&)));

    connect(_pool_monitor, SIGNAL(succeed())
        , this, SLOT(onPoolMonitorSucceed()));

    connect(_pool_monitor, SIGNAL(failed()), this, SLOT(onPoolMonitorFailed()));
    connect(_pool_monitor, SIGNAL(failed()), _config_handler, SLOT(nextPool()));

    AUnixSignalHandler *unix_sig_handler = new AUnixSignalHandler(this);
    connect(unix_sig_handler, SIGNAL(sigterm()), this, SLOT(stop()));
    connect(unix_sig_handler, SIGNAL(sigterm()), qApp, SLOT(quit()));
}


// ========================================================================== //
// Функция возврата директории.
// ========================================================================== //
QString AProxyMachine::workPath() const {return _work_path;}


// ========================================================================== //
// Функция установки директории.
// ========================================================================== //
void AProxyMachine::setWorkPath(const QString &path) {
    if(!path.isEmpty() && QDir(path).exists()) {
        const bool active = _active;
        if(active) stop();

        _work_path = path;

        if(_work_path.length() > 1 && _work_path.endsWith('/'))
            _work_path.remove(_work_path.length()-1,1);

        if(active) start();
    }
}


// ========================================================================== //
// Слот активации машины.
// ========================================================================== //
void AProxyMachine::start() {
    const QString pid_fname = QString("%1/stratumproxy.pid").arg(_work_path);
    if(QFile::exists(pid_fname)) {
        QFile pid_file(pid_fname);
        if(pid_file.open(QFile::ReadOnly)) {
            bool ok = false;
            const int pid = pid_file.readAll().toInt(&ok);
            if(ok && pid) {
                QProcess *pid_process = new QProcess(this);
                pid_process->start("ps -p " + QString::number(pid));
                pid_process->waitForFinished();
                if(pid_process->readAll().contains(QByteArray::number(pid))) {
                    logWarn("allready started"); qApp->quit(); return;
                }
            }

            pid_file.close();
        }

        pid_file.remove();
    }

    QFile pid_file(QString("%1/stratumproxy.pid").arg(_work_path));
    if(!pid_file.open(QFile::WriteOnly)) {
        logCrit("unable to create pid file"); qApp->quit(); return;
    }

    pid_file.write(QByteArray::number(qApp->applicationPid()));
    pid_file.close();

    _config_handler->setFileName(
        QString("%1/stratumproxy.ini").arg(_work_path));
    _config_handler->start();

    _active = true; logInfo("proxy started");
}


// ========================================================================== //
// Слот деактивации машины.
// ========================================================================== //
void AProxyMachine::stop() {
    _pool_monitor->stop(); _config_handler->stop(); onPoolMonitorFailed();

    QFile pid_file(QString("%1/stratumproxy.pid").arg(_work_path));
    if(pid_file.exists()) pid_file.remove();

    _active = false; logInfo("proxy stopped");
}


// ========================================================================== //
// Слот реакции на подключение пула.
// ========================================================================== //
void AProxyMachine::onPoolMonitorSucceed() {
    if(_pool_monitor->hasPoolChanged()) {
        if(_server) onPoolMonitorFailed();

        _server = new QTcpServer(this);

        connect(_server, SIGNAL(newConnection())
            , this, SLOT(onServerNewConnection()));

        if(!_server->listen(QHostAddress::Any
            , qApp->property("server-port").toInt()))
            logCrit("server is not listen!");

        logInfo("server is listen");
    }
}


// ========================================================================== //
// Слот реакции на отключение пула.
// ========================================================================== //
void AProxyMachine::onPoolMonitorFailed() {
    _pool_data_list.clear();

    if(_server) {
        if(_server->isListening()) _server->close();

        _server->deleteLater(); _server = NULL;

        logInfo("server closed");
    }
}


// ========================================================================== //
// Слот открытия нового соединения с сервером.
// ========================================================================== //
void AProxyMachine::onServerNewConnection() {
    if(!_server) return;

    QTcpSocket *miner_socket = _server->nextPendingConnection();
    if(!miner_socket) return;

    connect(miner_socket, SIGNAL(readyRead())
        , this, SLOT(onMinerSocketReadyRead()));
    connect(miner_socket, SIGNAL(disconnected())
        , this, SLOT(onMinerSocketDisconnected()));
    connect(miner_socket, SIGNAL(error(QAbstractSocket::SocketError))
        , this, SLOT(onMinerSocketError()));

    QTcpSocket *pool_socket = new QTcpSocket(_server);
    pool_socket->setProperty("miner_socket"
        , QVariant::fromValue((QObject*)miner_socket));

    connect(pool_socket, SIGNAL(connected())
        , this, SLOT(onPoolSocketConnected()));
    connect(pool_socket, SIGNAL(readyRead())
        , this, SLOT(onPoolSocketReadyRead()));
    connect(pool_socket, SIGNAL(error(QAbstractSocket::SocketError))
        , this, SLOT(onPoolSocketError()));

    pool_socket->connectToHost(_pool_monitor->host(), _pool_monitor->port());

    miner_socket->setProperty("pool_socket"
        , QVariant::fromValue((QObject*)pool_socket));
}


// ========================================================================== //
// Слот приёма сетевых сообщений от майнеров.
// ========================================================================== //
void AProxyMachine::onMinerSocketReadyRead() {
    QTcpSocket *miner_socket = qobject_cast<QTcpSocket*>(sender());
    if(!miner_socket) return;

    QVariant socket_var = miner_socket->property("pool_socket");
    if(socket_var.isValid() && socket_var.canConvert<QTcpSocket*>()) {
        QTcpSocket *pool_socket = socket_var.value<QTcpSocket*>();
        if(pool_socket->state() == QAbstractSocket::ConnectedState) {
            pool_socket->write(miner_socket->readAll());

        } else {
            _pool_data_list.append(
                qMakePair(pool_socket, miner_socket->readAll()));
        }
    }
}


// ========================================================================== //
// Слот закрытия сетевых соединений с майнером и пулом.
// ========================================================================== //
void AProxyMachine::onMinerSocketDisconnected() {
    QTcpSocket *miner_socket = qobject_cast<QTcpSocket*>(sender());
    if(!miner_socket) return;

    QVariant socket_var = miner_socket->property("pool_socket");
    if(socket_var.isValid() && socket_var.canConvert<QTcpSocket*>()) {
        QTcpSocket *pool_socket = socket_var.value<QTcpSocket*>();
        pool_socket->disconnectFromHost();
        pool_socket->deleteLater();
    }

    miner_socket->deleteLater();
}


// ========================================================================== //
// Слот обработки ошибок сетевой передачи данных майнера.
// ========================================================================== //
void AProxyMachine::onMinerSocketError() {
    QTcpSocket *miner_socket = qobject_cast<QTcpSocket*>(sender());
    if(!miner_socket) return;

    QVariant socket_var = miner_socket->property("pool_socket");
    if(socket_var.isValid() && socket_var.canConvert<QTcpSocket*>()) {
        QTcpSocket *pool_socket = socket_var.value<QTcpSocket*>();
        if(pool_socket->state() == QAbstractSocket::ConnectedState)
            pool_socket->abort();

        pool_socket->deleteLater();
    }

    miner_socket->abort(); miner_socket->deleteLater();
}


// ========================================================================== //
// Слот подключения пула.
// ========================================================================== //
void AProxyMachine::onPoolSocketConnected() {
    QTcpSocket *pool_socket = qobject_cast<QTcpSocket*>(sender());
    if(!pool_socket) return;

    QMutableListIterator<QPair<QTcpSocket*,QByteArray> > iter(_pool_data_list);
    while(iter.hasNext()) {
        QPair<QTcpSocket*,QByteArray> &pair = iter.next();
        if(pair.first == pool_socket) {
            pair.first->write(pair.second); iter.remove();
        }
    }
}


// ========================================================================== //
// Слот приёма сетевых сообщений от пула.
// ========================================================================== //
void AProxyMachine::onPoolSocketReadyRead() {
    QTcpSocket *pool_socket = qobject_cast<QTcpSocket*>(sender());
    if(!pool_socket) return;

    QVariant socket_var = pool_socket->property("miner_socket");
    if(socket_var.isValid() && socket_var.canConvert<QTcpSocket*>()) {
        QTcpSocket *miner_socket = socket_var.value<QTcpSocket*>();
        if(miner_socket->state() == QAbstractSocket::ConnectedState)
            miner_socket->write(pool_socket->readAll());
    }
}


// ========================================================================== //
// Слот обработки ошибок сетевой передачи данных пула.
// ========================================================================== //
void AProxyMachine::onPoolSocketError() {
    QTcpSocket *pool_socket = qobject_cast<QTcpSocket*>(sender());
    if(!pool_socket) return;

    QVariant socket_var = pool_socket->property("miner_socket");
    if(socket_var.isValid() && socket_var.canConvert<QTcpSocket*>()) {
        QTcpSocket *miner_socket = socket_var.value<QTcpSocket*>();
        if(miner_socket->state() == QAbstractSocket::ConnectedState)
            miner_socket->abort();

        miner_socket->deleteLater();
    }

    pool_socket->abort(); pool_socket->deleteLater();
}
