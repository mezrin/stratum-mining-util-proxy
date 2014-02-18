#ifndef ALOGGER_H
#define ALOGGER_H

#include <QtCore/QObject>

#include "asingleton.h"

class ALogger : public QObject, public ASingleton<ALogger> {
    Q_OBJECT

    friend class ASingleton<ALogger>;

    signals:
        void sigInfo(const QString &msg);
        void sigWarn(const QString &msg);
        void sigCrit(const QString &msg);

    public:
        enum MsgType {MSG_TYPE_INFO, MSG_TYPE_WARN, MSG_TYPE_CRIT};

        //! Функция установки сообщения.
        void info(const char *msg);

        //! Функция установки предупреждения.
        void warn(const char *msg);

        //! Функция установки ошибки.
        void crit(const char *msg);

        //! Функция установки флага вывода сообщений на консоль.
        void setHasTerminalLog(bool has_terminal_log);

        //! Функция установки файла логирования.
        void setFileName(const QString &fname);

    protected:
        //! Конструктор.
        explicit ALogger(QObject *parent = NULL);

        //! Деструктор.
        virtual ~ALogger() {}

    private:
        bool _has_terminal_log;

        QString _fname;

        //! Функция сохранения сообщения в файл.
        void save(const QString &msg);

};

static inline void logInfo(const char *msg) {ALogger::instance().info(msg);}
static inline void logInfo(const QString &msg) {
    ALogger::instance().info(qPrintable(msg));
}

static inline void logWarn(const char *msg) {ALogger::instance().warn(msg);}
static inline void logWarn(const QString &msg) {
    ALogger::instance().warn(qPrintable(msg));
}

static inline void logCrit(const char *msg) {ALogger::instance().crit(msg);}
static inline void logCrit(const QString &msg) {
    ALogger::instance().crit(qPrintable(msg));
}

#endif
