#ifndef AMAINPOOLHANDLER_H
#define AMAINPOOLHANDLER_H

#include <QtCore/QObject>

class QTimer;

class AMainPoolHandler : public QObject {
    Q_OBJECT

    signals:
        void poolChanged(const QString &pool);

        void checkingIntervalChanged(int interval);
        void checkingTimeoutChanged(int interval);

    public:
        //! Конструктор.
        explicit AMainPoolHandler(QObject *parent = NULL);

        //! Деструктор.
        virtual ~AMainPoolHandler() {}

        //! Функция возврата наименования файла конфигурации.
        QString fileName() const;

        //! Функция установки файла конфигурации по его наименованию.
        void setFileName(const QString &fname);

    public slots:
        //! Слот активации контроллера.
        void start();

        //! Слот деактивации контроллера.
        void stop();

    private:
        QTimer *_timer;

        int _checking_timeout;

        QString _fname, _pool;

    private slots:
        //! Слот активации таймера.
        void onTimerTimeout();

};

#endif
