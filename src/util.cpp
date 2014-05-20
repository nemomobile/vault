/**
 * @file util.cpp
 * @brief Misc. helpful utilities
 * @author Denis Zalevskiy <denis.zalevskiy@jolla.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <qtaround/util.hpp>
#include <qtaround/debug.hpp>

#include <QString>
#include <QMap>

namespace util {

namespace {

long capacityUnitExponent(QString const &name)
{
    static const QMap<QChar, long> multipliers = {
        {'k', 1}, {'m', 2}, {'g', 3}, {'t', 4}, {'p', 5}
        , {'e', 6}, {'z', 7}, {'y', 8} };
    static const QRegExp unit_re("^([kmgtpezy]?)i?b?$");
    auto suffix = name.toLower().trimmed();
    if (!unit_re.exactMatch(suffix))
        error::raise({{"msg", "Wrong bytes unit format"}, {"suffix", suffix}});

    if (name == QChar('b'))
        return 0;

    auto exp = multipliers.value(suffix[0], -1);
    if (exp == -1)
        error::raise({{"msg", "Wrong bytes unit multiplier"}
                      , {"suffix", suffix}});
    return exp;
}

}

double parseBytes(QString const &s, QString const &unit, long multiplier)
{
    debug::debug("parseBytes", s, unit);
    auto value = s.trimmed();
    static const QRegExp not_num_re("[^0-9.,]");
    auto num_end = value.indexOf(not_num_re);
    double res;
    bool ok = false;
    if (num_end == -1) {
        res = value.toDouble(&ok);
    } else {
        res = value.left(num_end).toDouble(&ok);
        auto exp = capacityUnitExponent(value.mid(num_end));

        if (unit != "b" && unit != "B")
            exp -= capacityUnitExponent(unit);

        res = res * pow(multiplier, exp);
    }
    if (!ok)
        error::raise({{"msg", "Can't parse bytes"}, {"value", value}});
    return res;
}

QVariant visit(visitor_type visitor, QVariant const &src, QVariant const &ctx)
{
    visitor_type visit_obj;
    visit_obj = [&visitor, &visit_obj](QVariant const &ctx
                                       , QVariant const &key
                                       , QVariant const &src) {
        auto process_obj = [visit_obj]
        (QVariant const &ctx, QVariant const &src) {
            if (hasType(src, QMetaType::QVariantMap)) {
                auto m = src.toMap();
                for (auto it = m.begin(); it != m.end(); ++it)
                    visit_obj(ctx, it.key(), it.value());
            } else if (hasType(src, QMetaType::QVariantList)) {
                auto l = src.toList();
                int i = 0;
                for (auto it = l.begin(); it != l.end(); ++it, ++i)
                    visit_obj(ctx, i, *it);
            }
            return ctx;
        };
        return process_obj(visitor(ctx, key, src), src);
    };
    return visit_obj(ctx, QVariant(), src);
}


}
