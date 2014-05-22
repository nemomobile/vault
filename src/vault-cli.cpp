/**
 * @file vault-cli.cpp
 * @brief Vault command line tool
 * @author Giulio Camuffo <giulio.camuffo@jollamobile.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <QCoreApplication>
#include <QDebug>

#include <vault/vault.hpp>
#include <qtaround/sys.hpp>

void set(QVariantMap &map, const sys::GetOpt &parser, const QString &option, bool optional = false)
{
    if (!optional || parser.isSet(option)) {
        map.insert(option, parser.value(option));
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    auto cmdline = sys::getopt({
        { "vault", QVariantMap{ {"short", "V"}, {"long", "vault"}, {"has_param", true} }},
        { "global", QVariantMap{ {"short", "G"}, {"long", "global"} }},
        { "action", QVariantMap{ {"short", "a"}, {"long", "action"}, {"has_param", true}, {"required", true} }},
        { "home", QVariantMap{ {"short", "H"}, {"long", "home"}, {"has_param", true} }},
        { "git-config", QVariantMap{ {"short", "g"}, {"long", "git-config"}, {"has_param", true} }},
        { "config-path", QVariantMap{ {"short", "c"}, {"long", "config-path"},
                           {"has_param", true}, {"default", "/etc/the-vault.json"} }},
        { "message", QVariantMap{ {"short", "m"}, {"long", "message"}, {"has_param", true} }},
        { "tag", QVariantMap{ {"short", "t"}, {"long", "tag"}, {"has_param", true} }},
        { "unit", QVariantMap{ {"short", "M"}, {"long", "unit"}, {"has_param", true} }},
        { "data", QVariantMap{ {"short", "d"}, {"long", "data"}, {"has_param", true} }}
    }, false);


    QVariantMap options;
    set(options, *cmdline, "action");
    set(options, *cmdline, "vault", true);
    set(options, *cmdline, "home", true);
    set(options, *cmdline, "data", true);
    set(options, *cmdline, "unit", true);
    set(options, *cmdline, "config-path", true);
    set(options, *cmdline, "git-config", true);
    set(options, *cmdline, "message", true);
    set(options, *cmdline, "tag", true);

    options.insert("global", cmdline->isSet("global"));

    vault::Vault::execute(options);

    return 0;
}
