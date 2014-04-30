#ifndef _CUTES_UTIL_HPP_
#define _CUTES_UTIL_HPP_

#include <QVariant>
#include <QDebug>
#include <QTextStream>

#include <tuple>
#include <memory>

static inline QVariantMap map(std::initializer_list<std::pair<QString, QVariant> > const &data)
{
    return QVariantMap(data);
}

static inline void insert(QVariantMap &dst, QVariantMap const &src)
{
    for (auto it = src.begin(); it != src.end(); ++it)
        dst.insert(it.key(), it.value());
}

static inline QVariant get(QVariantMap const &m, QString const &k1)
{
    return m[k1];
}

template <typename ... A>
QVariant get(QVariantMap const &m, QString const &k1, QString const &k2, A&& ...args)
{
    auto const &v = m[k1];
    return get(v.toMap(), k2, std::forward<A>(args)...);
}

namespace util {


}

#endif // _CUTES_UTIL_HPP_
