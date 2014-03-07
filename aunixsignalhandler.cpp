#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>

#include <QtCore/QSocketNotifier>

#include "aunixsignalhandler.h"

static int _g_sig_term_fd[2];

// ========================================================================== //
// Функция сигнала запроса завершения процесса.
// ========================================================================== //
void AUnixSignalHandler::sigTermHandler(int) {
    char a = 1;
    ssize_t r = ::write(_g_sig_term_fd[0], &a, sizeof(a));
    Q_UNUSED(r);
}


// ========================================================================== //
// Конструктор.
// ========================================================================== //
AUnixSignalHandler::AUnixSignalHandler(QObject *parent)
    : QObject(parent), _sig_term_notifier(NULL) {

    struct sigaction term;
    term.sa_handler = AUnixSignalHandler::sigTermHandler;
    sigemptyset(&term.sa_mask);
    term.sa_flags |= SA_RESTART;

    if(sigaction(SIGTERM, &term, 0) == 0) {
        if(::socketpair(AF_UNIX, SOCK_STREAM, 0, _g_sig_term_fd))
            qFatal("Couldn't create TERM socketpair");

        _sig_term_notifier
            = new QSocketNotifier(_g_sig_term_fd[1], QSocketNotifier::Read
                , this);
        connect(_sig_term_notifier, SIGNAL(activated(int))
            , this, SLOT(onSigTermHandle()));
    }
}


// ========================================================================== //
// Слот сигнала запроса завершения процесса.
// ========================================================================== //
void AUnixSignalHandler::onSigTermHandle() {
    if(!_sig_term_notifier) return;

    _sig_term_notifier->setEnabled(false);

    char tmp;
    ssize_t r = ::read(_g_sig_term_fd[1], &tmp, sizeof(tmp));
    Q_UNUSED(r);

    _sig_term_notifier->setEnabled(true);

    emit sigterm();
}
