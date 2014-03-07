#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QFile>
#include <QtCore/QDir>

#include "aproxymachine.h"
#include "aunixsignalhandler.h"
#include "aconfighandler.h"
#include "alogger.h"

// ========================================================================== //
// Конструктор.
// ========================================================================== //
AProxyMachine::AProxyMachine(QObject *parent)
    : QObject(parent), _active(false), _config_handler(new AConfigHandler(this))
    , _work_path(QCoreApplication::applicationDirPath()) {

    //connect(_config_handler, SIGNAL(poolChanged()), _state_machine, SLOT(stop()));

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

    _active = true;
}


// ========================================================================== //
// Слот деактивации машины.
// ========================================================================== //
void AProxyMachine::stop() {
    QFile pid_file(QString("%1/stratumproxy.pid").arg(_work_path));
    if(pid_file.exists()) pid_file.remove();

    _active = false;
}
