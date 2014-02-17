#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#include <QtCore/QFile>

#include <QtNetwork/QTcpServer>

#include "adaemon.h"

static int _g_sig_hup_fd[2], _g_sig_term_fd[2];

// ========================================================================== //
// Функция сигнала потери соединения с управляющим терминалом.
// ========================================================================== //
void ADaemon::sigHupHandler(int) {
    char a = 1;
    ssize_t r = ::write(_g_sig_hup_fd[0], &a, sizeof(a));
    Q_UNUSED(r);
}


// ========================================================================== //
// Функция сигнала запроса завершения процесса.
// ========================================================================== //
void ADaemon::sigTermHandler(int) {
    char a = 1;
    ssize_t r = ::write(_g_sig_term_fd[0], &a, sizeof(a));
    Q_UNUSED(r);
}


// ========================================================================== //
// Конструктор.
// ========================================================================== //
ADaemon::ADaemon(QObject *parent)
    : QObject(parent), _sig_hup_socket_notifier(NULL)
    , _sig_term_socket_notifier(NULL), _server_port(3400)
    , _pool_host("localhost"), _pool_port(3337) {

    struct sigaction hup;
    hup.sa_handler = ADaemon::sigHupHandler;
    sigemptyset(&hup.sa_mask);
    hup.sa_flags = 0;
    hup.sa_flags |= SA_RESTART;

    if(sigaction(SIGHUP, &hup, 0) == 0) {
        if(::socketpair(AF_UNIX, SOCK_STREAM, 0, _g_sig_hup_fd))
            qFatal("Couldn't create HUP socketpair");

        _sig_hup_socket_notifier
            = new QSocketNotifier(_g_sig_hup_fd[1], QSocketNotifier::Read
                , this);
        connect(_sig_hup_socket_notifier, SIGNAL(activated(int))
            , this, SLOT(onSigHupHandle()));
    }

    struct sigaction term;
    term.sa_handler = ADaemon::sigTermHandler;
    sigemptyset(&term.sa_mask);
    term.sa_flags |= SA_RESTART;

    if(sigaction(SIGTERM, &term, 0) == 0) {
        if(::socketpair(AF_UNIX, SOCK_STREAM, 0, _g_sig_term_fd))
            qFatal("Couldn't create TERM socketpair");

        _sig_term_socket_notifier
            = new QSocketNotifier(_g_sig_term_fd[1], QSocketNotifier::Read
                , this);
        connect(_sig_term_socket_notifier, SIGNAL(activated(int))
            , this, SLOT(onSigTermHandle()));
    }

    _config_timer = new QTimer(this);
    _config_timer->setInterval(60000);
    _config_timer->setSingleShot(false);

    connect(_config_timer, SIGNAL(timeout())
        , this, SLOT(onLoadConfiguration()));

    _config_timer->start();

    _server = new QTcpServer(this);
    _server->setMaxPendingConnections(100);

    connect(_server, SIGNAL(newConnection())
        , this, SLOT(onServerNewConnection()));
}


// ========================================================================== //
// Функция установки интервала чтения конфигурационного файла.
// ========================================================================== //
void ADaemon::setConfigReaderInterval(int minutes) {
    if(minutes > 0) _config_timer->setInterval(minutes * 60000);
}


// ========================================================================== //
// Функция установки порта сервера.
// ========================================================================== //
void ADaemon::setServerPort(int port) {
    if(port > 0) _server_port = port;
}


// ========================================================================== //
// Слот активации сервера.
// ========================================================================== //
void ADaemon::onListen() {
    if(_server->isListening()) {_server->close(); _pool_data_list.clear();}

    if(!_server->listen(QHostAddress::Any, _server_port))
        qFatal("Server is not listen!");
}


// ========================================================================== //
// Слот сигнала потери соединения с управляющим терминалом.
// ========================================================================== //
void ADaemon::onSigHupHandle() {
    _sig_hup_socket_notifier->setEnabled(false);

    char tmp;
    ssize_t r = ::read(_g_sig_hup_fd[1], &tmp, sizeof(tmp));
    Q_UNUSED(r);

    _sig_hup_socket_notifier->setEnabled(true);

    emit sighup();
}


// ========================================================================== //
// Слот сигнала запроса завершения процесса.
// ========================================================================== //
void ADaemon::onSigTermHandle() {
    _sig_term_socket_notifier->setEnabled(false);

    char tmp;
    ssize_t r = ::read(_g_sig_term_fd[1], &tmp, sizeof(tmp));
    Q_UNUSED(r);

    _sig_term_socket_notifier->setEnabled(true);

    emit sigterm();
}


// ========================================================================== //
// Слот загрузки конфигурационного файла.
// ========================================================================== //
void ADaemon::onLoadConfiguration() {
    QFile file(QCoreApplication::applicationDirPath() + "/stratumproxy.cfg");
    if(!file.open(QFile::ReadOnly)) return;

    QTextStream stream(&file);

    QString line;
    do {
        line = stream.readLine().trimmed();
        if(!line.startsWith('#') && line.contains(':')) {
            QString host = line.section(':', 0, 0);
            QString port = line.section(':', 1, 1);
            if(!host.isEmpty() && !port.isEmpty()) {
                bool ok = false;
                int p = port.toInt(&ok);
                if(ok) {
                    if(_pool_host != host || _pool_port != p) {
                        _pool_host = host; _pool_port = p;

                        QMetaObject::invokeMethod(this, "onListen"
                            , Qt::QueuedConnection);
                    }
                }

                break;
            }
        }

    } while(!line.isEmpty());

    file.close();
}


// ========================================================================== //
// Слот открытия нового соединения с сервером.
// ========================================================================== //
void ADaemon::onServerNewConnection() {
    QTcpSocket *miner_socket = _server->nextPendingConnection();
    if(!miner_socket) return;

    connect(miner_socket, SIGNAL(readyRead())
        , this, SLOT(onMinerSocketReadyRead()));
    connect(miner_socket, SIGNAL(disconnected())
        , this, SLOT(onMinerSocketDisconnected()));

    QTcpSocket *pool_socket = new QTcpSocket(_server);
    pool_socket->setProperty("miner_socket"
        , QVariant::fromValue((QObject*)miner_socket));

    connect(pool_socket, SIGNAL(connected())
        , this, SLOT(onPoolSocketConnected()));
    connect(pool_socket, SIGNAL(readyRead())
        , this, SLOT(onPoolSocketReadyRead()));
    connect(pool_socket, SIGNAL(error(QAbstractSocket::SocketError))
        , this, SLOT(onPoolSocketError()), Qt::QueuedConnection);

    pool_socket->connectToHost(_pool_host, _pool_port);

    miner_socket->setProperty("pool_socket"
        , QVariant::fromValue((QObject*)pool_socket));
}


// ========================================================================== //
// Слот приёма сетевых сообщений от майнеров.
// ========================================================================== //
void ADaemon::onMinerSocketReadyRead() {
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
void ADaemon::onMinerSocketDisconnected() {
    QTcpSocket *miner_socket = qobject_cast<QTcpSocket*>(sender());
    if(!miner_socket) return;

    QVariant socket_var = miner_socket->property("pool_socket");
    if(socket_var.isValid() && socket_var.canConvert<QTcpSocket*>()) {
        QTcpSocket *pool_socket = socket_var.value<QTcpSocket*>();
        pool_socket->disconnectFromHost();
        pool_socket->deleteLater();

        miner_socket->setProperty("pool_socket", QVariant());
    }

    miner_socket->deleteLater();
}


// ========================================================================== //
// Слот подключения пула.
// ========================================================================== //
void ADaemon::onPoolSocketConnected() {
    QTcpSocket *pool_socket = qobject_cast<QTcpSocket*>(sender());
    if(!pool_socket || _pool_data_list.isEmpty()) return;

    QMutableListIterator<QPair<QTcpSocket*,QByteArray> > iter(_pool_data_list);
    while(iter.hasNext()) {
        QPair<QTcpSocket*,QByteArray> &pair = iter.next();
        if(pair.first != pool_socket) continue;

        pair.first->write(pair.second);

        iter.remove(); break;
    }
}


// ========================================================================== //
// Слот приёма сетевых сообщений от пула.
// ========================================================================== //
void ADaemon::onPoolSocketReadyRead() {
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
void ADaemon::onPoolSocketError() {
    QTcpSocket *pool_socket = qobject_cast<QTcpSocket*>(sender());
    if(pool_socket) pool_socket->connectToHost(_pool_host, _pool_port);
}
