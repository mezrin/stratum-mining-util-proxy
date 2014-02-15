#ifndef ADAEMON_H
#define ADAEMON_H

#include <QtCore/QSocketNotifier>
#include <QtCore/QObject>

class ADaemon : public QObject {
    Q_OBJECT

    signals:
        void sighup();
        void sigterm();

    public:
        //! Функция сигнала потери соединения с управляющим терминалом.
        static void sigHupHandler(int);

        //! Функция сигнала запроса завершения процесса.
        static void sigTermHandler(int);

        //! Конструктор.
        explicit ADaemon(QObject *parent = NULL);

        //! Деструктор.
        virtual ~ADaemon() {}

    private:
        QSocketNotifier *_sig_hup_socket_notifier, *_sig_term_socket_notifier;

    private slots:
        //! Слот сигнала потери соединения с управляющим терминалом.
        void onSigHupHandle();

        //! Слот сигнала запроса завершения процесса.
        void onSigTermHandle();

};

#endif
