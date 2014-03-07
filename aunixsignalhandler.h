#ifndef AUNIXSIGNALHANDLER_H
#define AUNIXSIGNALHANDLER_H

#include <QtCore/QObject>

class QSocketNotifier;

class AUnixSignalHandler : public QObject {
    Q_OBJECT

    signals:
        void sigterm();

    public:
        //! Функция сигнала запроса завершения процесса.
        static void sigTermHandler(int);

        //! Конструктор.
        explicit AUnixSignalHandler(QObject *parent = NULL);

        //! Деструктор.
        virtual ~AUnixSignalHandler() {}

    private:
        QSocketNotifier *_sig_term_notifier;

    private slots:
        //! Слот сигнала запроса завершения процесса.
        void onSigTermHandle();

};

#endif
