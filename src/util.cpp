#include "util.hpp"

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
    auto value = s.trimmed();
    static const QRegExp not_num_re("[^0-9.,]");
    auto num_end = value.indexOf(not_num_re);
    double res;
    bool ok = false;
    if (num_end == -1) {
        res = value.toDouble(&ok);
    } else {
        res = value.left(num_end).toDouble(&ok);
        auto exp = capacityUnitExponent(value.right(num_end));

        if (unit != "b" && unit != "B")
            exp -= capacityUnitExponent(unit);

        multiplier ^= exp;
        res = res * multiplier;
    }
    if (!ok)
        error::raise({{"msg", "Can't parse bytes"}, {"value", value}});
    return res;
}


}
