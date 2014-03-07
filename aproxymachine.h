#ifndef APROXYMACHINE_H
#define APROXYMACHINE_H

#include <QtCore/QObject>

class AConfigHandler;

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

        QString _work_path;

};

#endif
