#ifndef APOOLMONITOR_H
#define APOOLMONITOR_H

#include <QtCore/QObject>

class APoolChecker;
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

        //! Функция возврата флага изменения пула.
        inline bool hasPoolChanged() const {return _has_pool_changed;}

        //! Функция возврата хоста.
        inline QString host() const {return _host;}

        //! Функция возврата порта.
        inline int port() const {return _port;}

    public slots:
        //! Слот установки пула на мониторинг.
        void changePool(const QString &pool);

        //! Слот установки интервала.
        void changeCheckingInterval(int interval);

        //! Слот установки интервала.
        void changeCheckingTimeout(int interval);

        //! Слот активации мониторинга.
        void start();

        //! Слот деактивации мониторинга.
        void stop();

    private:
        bool _has_pool_changed;

        QString _host;

        int _port;

        APoolChecker *_pool_checker;

        QTimer *_timer;

    private slots:
        //! Слот активации таймера.
        void onTimerTimeout();

};

#endif
