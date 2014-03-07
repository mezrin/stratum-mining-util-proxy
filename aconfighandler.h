#ifndef ACONFIGHANDLER_H
#define ACONFIGHANDLER_H

#include <QtCore/QObject>

class QTimer;

class AConfigHandler : public QObject {
    Q_OBJECT

    signals:
        void poolChanged();

    public:
        //! Конструктор.
        explicit AConfigHandler(QObject *parent = NULL);

        //! Деструктор.
        virtual ~AConfigHandler() {}

        //! Функция возврата наименования файла конфигурации.
        QString fileName() const;

        //! Функция установки файла конфигурации по его наименованию.
        void setFileName(const QString &fname);

        //! Функция возврата текущего пула.
        QString pool() const;

    public slots:
        //! Слот активации мониторинга файла конфигурации.
        void start();

        //! Слот деактивации мониторинга файла конфигурации.
        void stop();

        //! Слот переключения текущего пула.
        void nextPool();

    private:
        QTimer *_timer;

        QString _fname;

        int _pool_index;

        QString _pool;

        //! Функция сброса значений параметров.
        void reset();

    private slots:
        //! Слот активации таймера.
        void onTimerTimeout();

};

#endif
