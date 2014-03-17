#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QFile>
#include <QtCore/QDir>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include "aproxymachine.h"
#include "aunixsignalhandler.h"
#include "abackuppoolhandler.h"
#include "amainpoolhandler.h"
#include "aconfighandler.h"
#include "apoolmonitor.h"
#include "alogger.h"

// ========================================================================== //
// Конструктор.
// ========================================================================== //
AProxyMachine::AProxyMachine(QObject *parent)
    : QObject(parent), _active(false), _has_main_pool(true)
    , _config_handler(new AConfigHandler(this))
    , _main_pool_handler(new AMainPoolHandler(this))
    , _backup_pool_handler(new ABackupPoolHandler(this))
    , _main_pool_monitor(new APoolMonitor(this))
    , _backup_pool_monitor(new APoolMonitor(this)), _server(NULL)
    , _work_path(QCoreApplication::applicationDirPath()) {

    connect(_main_pool_handler, SIGNAL(poolChanged(const QString&))
        , this, SLOT(serverStop()));

    connect(_main_pool_handler, SIGNAL(poolChanged(const QString&))
        , _backup_pool_monitor, SLOT(stop()));
    connect(_main_pool_handler, SIGNAL(poolChanged(const QString&))
        , _backup_pool_handler, SLOT(stop()));

    connect(_main_pool_handler, SIGNAL(poolChanged(const QString&))
        , _main_pool_monitor, SLOT(stop()));
    connect(_main_pool_handler, SIGNAL(poolChanged(const QString&))
        , _main_pool_monitor, SLOT(changePool(const QString&)));
    connect(_main_pool_handler, SIGNAL(checkingIntervalChanged(int))
        , _main_pool_monitor, SLOT(changeCheckingInterval(int)));
    connect(_main_pool_handler, SIGNAL(checkingTimeoutChanged(int))
        , _main_pool_monitor, SLOT(changeCheckingTimeout(int)));

    connect(_main_pool_monitor, SIGNAL(succeed())
        , this, SLOT(onMainPoolSucceed()));
    connect(_main_pool_monitor, SIGNAL(failed())
        , this, SLOT(onMainPoolFailed()));

    connect(_backup_pool_handler, SIGNAL(poolChanged(const QString&))
        , _backup_pool_monitor, SLOT(stop()));
    connect(_backup_pool_handler, SIGNAL(poolChanged(const QString&))
        , _backup_pool_monitor, SLOT(changePool(const QString&)));
    connect(_backup_pool_handler, SIGNAL(checkingIntervalChanged(int))
        , _backup_pool_monitor, SLOT(changeCheckingInterval(int)));
    connect(_backup_pool_handler, SIGNAL(checkingTimeoutChanged(int))
        , _backup_pool_monitor, SLOT(changeCheckingTimeout(int)));

    connect(_backup_pool_monitor, SIGNAL(succeed())
        , this, SLOT(onBackupPoolSucceed()));
    connect(_backup_pool_monitor, SIGNAL(failed())
        , this, SLOT(onBackupPoolFailed()));

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

    logInfo("proxy machine started");

    const QString fname = QString("%1/stratumproxy.ini").arg(_work_path);

    _config_handler->setFileName(fname);
    _main_pool_handler->setFileName(fname);
    _backup_pool_handler->setFileName(fname);

    _main_pool_handler->start();

    _active = true;
}


// ========================================================================== //
// Слот деактивации машины.
// ========================================================================== //
void AProxyMachine::stop() {
    _backup_pool_monitor->stop(); _backup_pool_handler->stop();

    _main_pool_monitor->stop(); _main_pool_handler->stop();

    serverStop();

    QFile pid_file(QString("%1/stratumproxy.pid").arg(_work_path));
    if(pid_file.exists()) pid_file.remove();

    _active = false; logInfo("proxy machine stopped");
}


// ========================================================================== //
// Слот активации сервера.
// ========================================================================== //
void AProxyMachine::serverStart() {
    if(_server) serverStop();

    _server = new QTcpServer(this);
    connect(_server, SIGNAL(newConnection())
        , this, SLOT(onServerNewConnection()));

    if(!_server->listen(QHostAddress::Any
        , qApp->property("server-port").toInt()))
        logCrit("server is not listen!");

    logInfo("server is listen");
}


// ========================================================================== //
// Слот деактивации сервера.
// ========================================================================== //
void AProxyMachine::serverStop() {
    _pool_data_list.clear();

    if(_server) {
        if(_server->isListening()) _server->close();

        _server->deleteLater(); _server = NULL;

        logInfo("server closed");
    }
}


// ========================================================================== //
// Слот подключения главного пула.
// ========================================================================== //
void AProxyMachine::onMainPoolSucceed() {
    if(!_has_main_pool) {
        _has_main_pool = true; serverStop();

        _backup_pool_monitor->stop(); _backup_pool_handler->stop();

        serverStart(); return;
    }

    if(_main_pool_monitor->hasPoolChanged()) serverStart();
}


// ========================================================================== //
// Слот отключения главного пула.
// ========================================================================== //
void AProxyMachine::onMainPoolFailed() {
    if(!_has_main_pool) return;

    _has_main_pool = false; serverStop(); _backup_pool_handler->start();
}


// ========================================================================== //
// Слот подключения бэкап пула.
// ========================================================================== //
void AProxyMachine::onBackupPoolSucceed() {
    if(_has_main_pool) {
        _backup_pool_monitor->stop(); _backup_pool_handler->stop(); return;
    }

    if(_backup_pool_monitor->hasPoolChanged()) serverStart();
}


// ========================================================================== //
// Слот отключения бэкап пула.
// ========================================================================== //
void AProxyMachine::onBackupPoolFailed() {
    if(_has_main_pool) {
        _backup_pool_monitor->stop(); _backup_pool_handler->stop(); return;
    }

    serverStop();

    _backup_pool_monitor->stop(); _backup_pool_handler->nextPool();
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

    switch(_has_main_pool) {
        case true:
            pool_socket->connectToHost(_main_pool_monitor->host()
                , _main_pool_monitor->port());
        break;
        case false:
            pool_socket->connectToHost(_backup_pool_monitor->host()
                , _backup_pool_monitor->port());
        break;
    }

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
