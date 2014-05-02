
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>

#include "vault.hpp"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("The Vault");
    parser.addHelpOption();

    QCommandLineOption actionOption(QStringList() << "a" << "action", "action", "action");
    parser.addOption(actionOption);
    QCommandLineOption vaultOption(QStringList() << "v" << "vault", "vault", "path");
    parser.addOption(vaultOption);

    parser.process(app);

    Vault vault(parser.value("vault"));
    QString action = parser.value("action");
    if (action == "init") {
        qDebug()<<vault.init();
    }


    qDebug()<<parser.value("action");

    return 0;
}
