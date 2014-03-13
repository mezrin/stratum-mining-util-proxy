#include <sys/types.h>
#include <sys/stat.h>

#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QTextCodec>
#include <QtCore/QLocale>
#include <QtCore/QDir>

#include "aproxymachine.h"
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

    QCommandLineParser cmd_line_parser;
    cmd_line_parser.setApplicationDescription("stratumproxy");
    cmd_line_parser.addHelpOption();

    QCommandLineOption terminal_option(
        QStringList() << "t" << "terminal"
            , QCoreApplication::translate("main"
                , "Start application in interactive mode."));

    QCommandLineOption work_path_option(
        QStringList() << "w" << "work-path"
            , QCoreApplication::translate("main"
                , "Set working path into <path>.")
            , QCoreApplication::translate("main", "path"));

    cmd_line_parser.addOption(terminal_option);
    cmd_line_parser.addOption(work_path_option);
    cmd_line_parser.process(app);

    if(cmd_line_parser.isSet("h") || cmd_line_parser.isSet("help"))
        return app.exec();

    QString work_path = QCoreApplication::applicationDirPath();
    if(cmd_line_parser.isSet(work_path_option)) {
        QString path = cmd_line_parser.value(work_path_option);
        if(QDir(path).exists()) work_path = path;
    }

    ALogger::instance().setFileName(work_path + "/stratumproxy.log");
    ALogger::instance()
        .setHasTerminalLog(cmd_line_parser.isSet(terminal_option));

    AProxyMachine proxy_machine(&app);
    proxy_machine.setWorkPath(work_path);

    QMetaObject::invokeMethod(&proxy_machine, "start", Qt::QueuedConnection);

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
