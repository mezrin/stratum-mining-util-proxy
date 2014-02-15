#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>

#include <QtCore/QCoreApplication>

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
    , _sig_term_socket_notifier(NULL) {

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
