#ifndef _CUTES_UTIL_HPP_
#define _CUTES_UTIL_HPP_

#include <cor/util.hpp>

#include <QVariant>
#include <QDebug>
#include <QTextStream>
#include <QMap>

#include <tuple>
#include <memory>

namespace {

inline QString str(QVariant const &v)
{
    return v.toString();
}

typedef QVariantMap map_type;
typedef QMap<QString, QString> string_map_type;

inline QVariantMap map(std::initializer_list<std::pair<QString, QVariant> > data)
{
    return QVariantMap(data);
}

inline QVariantMap const & map(QVariantMap const &data)
{
    return data;
}

void insert(QVariantMap &dst, QVariantMap const &src)
{
    for (auto it = src.begin(); it != src.end(); ++it)
        dst.insert(it.key(), it.value());
}

QVariant get(QVariantMap const &m, QString const &k1)
{
    return m[k1];
}

}
template <typename ... A>
QVariant get(QVariantMap const &m, QString const &k1, QString const &k2, A&& ...args)
{
    auto const &v = m[k1];
    return get(v.toMap(), k2, std::forward<A>(args)...);
}

template <typename T>
std::unique_ptr<T> box(T &&v)
{
    return cor::make_unique<T>(std::move(v));
}

template <typename T>
T unbox(std::unique_ptr<T> p)
{
    return std::move(*p);
}

namespace util {


}

#endif // _CUTES_UTIL_HPP_
