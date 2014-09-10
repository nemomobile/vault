/**
 * @file sys.cpp
 * @brief Command line parsing and generation etc.
 * @author Giulio Camuffo <giulio.camuffo@jollamobile.com>, Denis Zalevskiy <denis.zalevskiy@jolla.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <qtaround/sys.hpp>

#include <cor/util.hpp>

#include <QStringList>
#include <QCoreApplication>

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

    void addOption(const QString &shortName, const QString &longName,
                   const QString &description, const QString &defaultValue)
    {
        QStringList names = { QString("-") + shortName, QString("--") + longName };
        auto opt = std::make_shared<Option>(true, names, description, defaultValue);
        names.push_back(description);
        addOption(names, opt);
    }

    void addOption(const QString &shortName, const QString &longName,
                   const QString &description)
    {
        QStringList names = { QString("-") + shortName, QString("--") + longName };
        auto opt = std::make_shared<Option>(false, names, description);
        names.push_back(description);
        addOption(names, opt);
    }

    void process()
    {
        QStringList args = QCoreApplication::arguments();
        for (int i = 1; i < args.count(); ++i) {
            QString arg = args.at(i);
            bool hasEqual = arg.contains("=");
            if (hasEqual) {
                arg = arg.split("=").first();
            }
            for (auto opt: options_) {
                if (!opt->names.contains(arg)) {
                    continue;
                }

                if (!opt->hasParam) {
                    opt->set = true;
                    break;
                }

                bool valid = true;
                QString param;
                if (hasEqual) {
                    param = args.at(i).mid(arg.size() + 1);
                } else {
                    if (i == args.count() - 1) {
                        break;
                    }
                    param = args.at(i + 1);
                    for (auto o: options_) {
                        if (o->names.contains(param)) {
                            valid = false;
                            break;
                        }
                    }
                }
                opt->set = true;
                opt->value = valid ? param : opt->defaultValue;
            }
        }
    }

    virtual QString value(QString const &name) const
    {
        auto opt = optionsMap_.value(name);
        return opt ? opt->value : QString();
    }

    virtual bool isSet(QString const &name) const
    {
        auto opt = optionsMap_.value(name);
        return opt ? opt->set : false;
    }

    virtual QStringList arguments() const
    {
        return QStringList();
    }

private:
    friend std::unique_ptr<GetOpt> getopt(QVariantMap const &);

    struct Option {
        Option(bool hp, const QStringList &n, const QString &d, const QString &dv = QString())
            : hasParam(hp)
            , names(n)
            , description(d)
            , defaultValue(dv)
            , set(false)
        {
        }

        bool hasParam;
        QStringList names;
        QString description;
        QString defaultValue;

        QString value;
        bool set;
    };

    void addOption(QStringList const &names, std::shared_ptr<Option> const &opt)
    {
        options_.push_back(opt);
        for (auto const &name: names) {
            optionsMap_.insert(name, opt);
        }
    }

    QList<std::shared_ptr<Option> > options_;
    QMap<QString, std::shared_ptr<Option> > optionsMap_;
};

std::unique_ptr<GetOpt> getopt(QVariantMap const &info, bool requireAll)
{
    auto res = cor::make_unique<OptionsImpl>();

    auto add_option = [](OptionsImpl &parser
                    , QString const &name
                    , QVariant const &v) {
        auto info = v.toMap();
        if (info["has_param"].toBool()) {
           parser.addOption(str(info["short"]), str(info["long"]), name, name);
        } else {
            parser.addOption(str(info["short"]), str(info["long"]), name);
        }
    };
    for (auto it = info.begin(); it != info.end(); ++it) {
        add_option(*res, it.key(), it.value());
    }
    res->process();
    if (requireAll) {
        for (auto it = info.begin(); it != info.end(); ++it) {
            auto name = it.key();
            if (!res->isSet(name))
                error::raise({{"msg", "Required option is not set"}
                        , {"option", name}});
        }
    }

    return std::move(res);
}

}
