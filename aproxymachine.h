#ifndef APROXYMACHINE_H
#define APROXYMACHINE_H

#include <QtCore/QObject>
#include <QtCore/QPair>

class QTcpServer;
class QTcpSocket;

class AConfigHandler;
class APoolMonitor;

class AProxyMachine : public QObject {
    Q_OBJECT

    public:
        //! Конструктор.
        explicit AProxyMachine(QObject *parent = NULL);

        //! Деструктор.
        virtual ~AProxyMachine() {}

        //! Функция возврата флага активности машины.
        inline bool isActive() const {return _active;}

        //! Функция возврата директории.
        QString workPath() const;

        //! Функция установки директории.
        void setWorkPath(const QString &path);

    public slots:
        //! Слот активации машины.
        void start();

        //! Слот деактивации машины.
        void stop();

    private:
        bool _active;

        AConfigHandler *_config_handler;

        APoolMonitor *_pool_monitor;

        QTcpServer *_server;

        QString _work_path;

        QList<QPair<QTcpSocket*,QByteArray> > _pool_data_list;

    private slots:
        //! Слот реакции на подключение пула.
        void onPoolMonitorSucceed();

        //! Слот реакции на отключение пула.
        void onPoolMonitorFailed();

        //! Слот открытия нового соединения с сервером.
        void onServerNewConnection();

        //! Слот приёма сетевых сообщений от майнеров.
        void onMinerSocketReadyRead();

        //! Слот закрытия сетевых соединений с майнером и пулом.
        void onMinerSocketDisconnected();

        //! Слот обработки ошибок сетевой передачи данных майнера.
        void onMinerSocketError();

        //! Слот подключения пула.
        void onPoolSocketConnected();

        //! Слот приёма сетевых сообщений от пула.
        void onPoolSocketReadyRead();

        //! Слот обработки ошибок сетевой передачи данных пула.
        void onPoolSocketError();

};

#endif
