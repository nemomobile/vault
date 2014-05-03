#include "sys.hpp"
#include <QStringList>

namespace sys {
QStringList command_line_options(QVariantMap const &options
                                 , string_map_type const &short_options
                                 , string_map_type const &long_options
                                 , QMap<QString, bool> const &options_has_param)
{
    QStringList cmd_options;

    for (auto it = options.begin(); it != options.end(); ++it) {
        auto n = it.key();
        auto v = it.value();
        auto opt = short_options[n];
        if (!opt.isEmpty()) {
            if (options_has_param[n]) {
                cmd_options.append(QStringList({QString("-")+ opt, str(v)}));
            } else {
                if (v.toBool())
                    cmd_options.push_back(QString("-") + opt);
            }
            continue;
        }
        opt = long_options[n];
        if (!opt.isEmpty()) {
            if (options_has_param[n]) {
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
}
