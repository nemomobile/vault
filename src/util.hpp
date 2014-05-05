#ifndef _CUTES_UTIL_HPP_
#define _CUTES_UTIL_HPP_

#include "error.hpp"
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

inline bool is(QVariant const &v)
{
    return v.toBool();
}

inline bool hasType(QVariant const &v, QMetaType::Type t)
{
    return static_cast<QMetaType::Type>(v.type()) == t;
}

typedef QVariantMap map_type;
typedef QMap<QString, QString> string_map_type;

inline QVariantMap map(std::initializer_list<std::pair<QString, QVariant> > data)
{
    return QVariantMap(data);
}

inline QVariantList list(std::initializer_list<QVariant> data)
{
    return QVariantList(data);
}

inline QVariantMap map(QVariant const &v, bool check_type = true)
{
    if (check_type && !hasType(v, QMetaType::QVariantMap))
        error::raise({{"msg", "Need QVariantMap"}, {"value", v}});
    return v.toMap();
}

inline QVariantMap const & map(QVariantMap const &data)
{
    return data;
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
    std::unique_ptr<T> p(new T(std::move(v)));
    //return cor::make_unique<T>();
    return p;
}

template <typename T>
T unbox(std::unique_ptr<T> p)
{
    return std::move(*p);
}

namespace util {


template <typename ResT, typename T, typename FnT>
QList<ResT> map(QList<T> const &src, FnT fn)
{
    QList<ResT> res;
    for (auto it = src.begin(); it != src.end(); ++it) {
        res.push_back(fn(*it));
    }
    return res;
}

}

#endif // _CUTES_UTIL_HPP_
