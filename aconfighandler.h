#ifndef ACONFIGHANDLER_H
#define ACONFIGHANDLER_H

#include <QtCore/QObject>

class AConfigHandler : public QObject {
    Q_OBJECT

    public:
        //! Конструктор.
        explicit AConfigHandler(QObject *parent = NULL);

        //! Деструктор.
        virtual ~AConfigHandler() {}

        //! Функция возврата наименования файла конфигурации.
        QString fileName() const;

        //! Функция установки файла конфигурации по его наименованию.
        void setFileName(const QString &fname);

    private:
        QString _fname;

};

#endif
