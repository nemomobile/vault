#include "sys.hpp"

#include <QStringList>
#include <QCoreApplication>
#include <QCommandLineParser>

namespace sys {
QStringList command_line_options(QVariantMap const &options
                                 , string_map_type const &short_options
                                 , string_map_type const &long_options
                                 , QSet<QString> const &options_has_param)
{
    QStringList cmd_options;

    for (auto it = options.begin(); it != options.end(); ++it) {
        auto n = it.key();
        auto v = it.value();
        auto opt = short_options[n];
        if (!opt.isEmpty()) {
            if (options_has_param.contains(n)) {
                auto params = QStringList({QString("-")+ opt, str(v)});
                cmd_options.append(params);
            } else {
                if (v.toBool())
                    cmd_options.push_back(QString("-") + opt);
            }
            continue;
        }
        opt = long_options[n];
        if (!opt.isEmpty()) {
            if (options_has_param.contains(n)) {
                auto long_opt = QStringList({"--", opt, "=", str(v)}).join("");
                cmd_options.push_back(long_opt);
            } else {
                if (v.toBool())
                    cmd_options.push_back(QString("--") + opt);
            }
        }
    };
    return cmd_options;
}

class OptionsImpl : public GetOpt
{
public:
    OptionsImpl()
    {}

    virtual QString value(QString const &name) const
    {
        return parser_.value(name);
    }

    virtual bool isSet(QString const &name) const
    {
        return parser_.isSet(name);
    }

    virtual QStringList arguments() const
    {
        return parser_.positionalArguments();
    }

private:
    friend std::unique_ptr<GetOpt> getopt(QVariantMap const &);

    QCommandLineParser parser_;
};

std::unique_ptr<GetOpt> getopt(QVariantMap const &info)
{
    auto res = cor::make_unique<OptionsImpl>();
    QCommandLineParser &parser = res->parser_;
    parser.setApplicationDescription("Vault unit");
    parser.addHelpOption();

    auto add_option = [](QCommandLineParser &parser
                    , QString const &name
                    , QVariant const &v) {
        auto info = v.toMap();
        if (info["has_param"].toBool()) {
            QCommandLineOption o(QStringList() << str(info["short"])
                                 << str(info["long"])
                                 , name, name);
            parser.addOption(o);
        } else {
            QCommandLineOption o(QStringList() << str(info["short"])
                                 << str(info["long"]), name);
            parser.addOption(o);
        }
    };
    for (auto it = info.begin(); it != info.end(); ++it) {
        add_option(parser, it.key(), it.value());
    }
    parser.process(*QCoreApplication::instance());
    for (auto it = info.begin(); it != info.end(); ++it) {
        auto name = it.key();
        if (!parser.isSet(name))
            error::raise({{"msg", "Required option is not set"}
                    , {"option", name}});
    }
    return std::move(res);
}

}
