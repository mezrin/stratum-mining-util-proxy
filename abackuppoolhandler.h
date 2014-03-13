#ifndef ABACKUPPOOLHANDLER_H
#define ABACKUPPOOLHANDLER_H

#include <QtCore/QObject>

class QTimer;

class ABackupPoolHandler : public QObject {
    Q_OBJECT

    signals:
        void poolChanged(const QString &pool);

        void checkingIntervalChanged(int interval);

    public:
        //! Конструктор.
        explicit ABackupPoolHandler(QObject *parent = NULL);

        //! Деструктор.
        virtual ~ABackupPoolHandler() {}

        //! Функция возврата наименования файла конфигурации.
        QString fileName() const;

        //! Функция установки файла конфигурации по его наименованию.
        void setFileName(const QString &fname);

    public slots:
        //! Слот активации контроллера.
        void start();

        //! Слот деактивации контроллера.
        void stop();

        //! Слот переключения текущего пула.
        void nextPool();

    private:
        QTimer *_timer;

        QString _fname, _pool;

        int _pool_index;

    private slots:
        //! Слот активации таймера.
        void onTimerTimeout();

};

#endif
