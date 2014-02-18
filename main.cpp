#include <sys/types.h>
#include <sys/stat.h>

#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QTextCodec>
#include <QtCore/QLocale>

#include "adaemon.h"
#include "alogger.h"

// ========================================================================== //
// Функция запуска процесса.
// ========================================================================== //
int startProcess(int argc, char *argv[]) {
    QCoreApplication::setOrganizationName("AlexisSoft");
    QCoreApplication::setOrganizationDomain("free-lance.ru");
    QCoreApplication::setApplicationName("stratumproxy");

    QCoreApplication app(argc, argv);

    QLocale::setDefault(QLocale(QLocale::Russian, QLocale::RussianFederation));
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    ADaemon daemon(&app);
    QObject::connect(&daemon, SIGNAL(sigterm()), &app, SLOT(quit()));

    ALogger::instance();

    QCommandLineParser cmd_line_parser;
    cmd_line_parser.setApplicationDescription("stratumproxy");
    cmd_line_parser.addHelpOption();

    QCommandLineOption port_option(
        QStringList() << "p" << "server-port",
            QCoreApplication::translate("main"
                , "Number of network port, default 3400"),
            QCoreApplication::translate("main", "port"));

    QCommandLineOption checking_interval_option(
        QStringList() << "c" << "checking-interval",
            QCoreApplication::translate("main"
                , "Config file checking interval, default 1"),
            QCoreApplication::translate("main", "minutes"));

    QCommandLineOption terminal_option(
        QStringList() << "t" << "terminal",
            QCoreApplication::translate("main"
                , "Start application in interactive mode."));

    cmd_line_parser.addOption(port_option);
    cmd_line_parser.addOption(checking_interval_option);
    cmd_line_parser.addOption(terminal_option);
    cmd_line_parser.process(app);

    daemon.setServerPort(cmd_line_parser.value(port_option).toInt());
    daemon.setConfigReaderInterval(
        cmd_line_parser.value(checking_interval_option).toInt());

    return app.exec();
}


// ========================================================================== //
//
// ========================================================================== //
int main(int argc, char *argv[]) {
    bool has_terminal = false;
    for(int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        if(arg == "-t" || arg == "--terminal"
            || arg == "-h" || arg == "--help") {

            has_terminal = true; break;
        }
    }

    if(has_terminal) return startProcess(argc, argv);

    int pid = fork();
    if(pid == -1) return -1;

    if(!pid) {
        umask(0); setsid(); int r = chdir("/"); Q_UNUSED(r);

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        return startProcess(argc, argv);
    }

    return 0;
}
