#ifndef ADAEMON_H
#define ADAEMON_H

#include <QtCore/QSocketNotifier>
#include <QtCore/QObject>

#include <QtNetwork/QTcpSocket>

class QTcpServer;

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

    public slots:
        //! Слот активации сервера.
        void onListen();

    private:
        QSocketNotifier *_sig_hup_socket_notifier, *_sig_term_socket_notifier;

        QTcpServer *_server;

        int _server_port;

        QString _pool_host;

        int _pool_port;

        QList<QPair<QTcpSocket*,QByteArray> > _pool_data_list;

    private slots:
        //! Слот сигнала потери соединения с управляющим терминалом.
        void onSigHupHandle();

        //! Слот сигнала запроса завершения процесса.
        void onSigTermHandle();

        //! Слот открытия нового соединения с сервером.
        void onServerNewConnection();

        //! Слот приёма сетевых сообщений от майнеров.
        void onMinerSocketReadyRead();

        //! Слот закрытия сетевых соединений с майнером и пулом.
        void onMinerSocketDisconnected();

        //! Слот подключения пула.
        void onPoolSocketConnected();

        //! Слот приёма сетевых сообщений от пула.
        void onPoolSocketReadyRead();

        //! Слот обработки ошибок сетевой передачи данных пула.
        void onPoolSocketError();

};

#endif
