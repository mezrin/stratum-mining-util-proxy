#ifndef APOOLMONITOR_H
#define APOOLMONITOR_H

#include <QtCore/QObject>

class QTcpSocket;
class QTimer;

class APoolMonitor : public QObject {
    Q_OBJECT

    signals:
        void succeed();
        void failed();

    public:
        //! Конструктор.
        explicit APoolMonitor(QObject *parent = NULL);

        //! Деструктор.
        virtual ~APoolMonitor() {}

        //! Функция возврата флага активности монитора.
        inline bool isActive() const {return _active;}

        //! Функция установки хоста.
        void setHost(const QString &host);

        //! Функция установки хоста.
        void setHost(const QString &host, int port);

        //! Функция установки порта.
        void setPort(int port);

    public slots:
        //! Слот активации мониторинга.
        void start();

        //! Слот деактивации мониторинга.
        void stop();

    private:
        enum State {STATE_RDY_WRITE, STATE_RDY_READ};

        State _state;

        bool _active;

        QString _host;

        int _port;

        QTimer *_timer;

        QTcpSocket *_socket;

    private slots:
        //! Слот активации таймера.
        void onTimerTimeout();

        //! Слот приёма сетевых сообщений.
        void onSocketReadyRead();

        //! Слот обработки ошибок сетевой передачи данных.
        void onSocketError();

};

#endif
