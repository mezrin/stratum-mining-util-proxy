#ifndef APOOLCHECKER_H
#define APOOLCHECKER_H

#include <QtCore/QObject>

class QTcpSocket;
class QTimer;

class APoolChecker : public QObject {
    Q_OBJECT

    signals:
        void succeed();
        void failed();

    public:
        //! Конструктор.
        explicit APoolChecker(QObject *parent = NULL);

        //! Деструктор.
        virtual ~APoolChecker() {}

    public slots:
        //! Слот проверки активности пула.
        void check(const QString &host, int port);

    private:
        QTcpSocket *_socket;

        QTimer *_timer;

    private slots:
        //! Слот подключения сокета к хосту.
        void onSocketConnected();

        //! Слот приёма сетевых сообщений.
        void onSocketReadyRead();

        //! Слот обработки ошибок сетевой передачи данных.
        void onSocketError();

        //! Слот активации таймера.
        void onTimerTimeout();

};

#endif
