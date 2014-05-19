
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>

#include <vault/vault.hpp>

void set(QVariantMap &map, const QCommandLineParser &parser, const QString &option, bool optional = false)
{
    if (!optional || parser.isSet(option)) {
        map.insert(option, parser.value(option));
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("The Vault");
    parser.addHelpOption();

    parser.addOption(QCommandLineOption(QStringList() << "a" << "action", "action", "action"));
    parser.addOption(QCommandLineOption(QStringList() << "V" << "vault", "vault", "path"));
    parser.addOption(QCommandLineOption(QStringList() << "H" << "home", "home", "home"));
    parser.addOption(QCommandLineOption(QStringList() << "G" << "global", "global"));
    parser.addOption(QCommandLineOption(QStringList() << "d" << "data", "data", "data"));
    parser.addOption(QCommandLineOption(QStringList() << "M" << "unit", "unit", "unit"));
    parser.addOption(QCommandLineOption(QStringList() << "c" << "config-path", "config-path", "config-path", "/etc/the-vault.json"));
    parser.addOption(QCommandLineOption(QStringList() << "g" << "git-config", "git-config", "git-config"));
    parser.addOption(QCommandLineOption(QStringList() << "m" << "message", "message", "message"));
    parser.addOption(QCommandLineOption(QStringList() << "t" << "tag", "tag", "tag"));

    parser.process(app);

    QVariantMap options;
    set(options, parser, "action");
    set(options, parser, "vault", true);
    set(options, parser, "home", true);
    set(options, parser, "data", true);
    set(options, parser, "unit", true);
    set(options, parser, "config-path", true);
    set(options, parser, "git-config", true);
    set(options, parser, "message", true);
    set(options, parser, "tag", true);

    options.insert("global", parser.isSet("global"));

    vault::Vault::execute(options);

    return 0;
}
