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

inline QString str(QByteArray const &v)
{
    return QString::fromUtf8(v);
}

inline QString str(int v)
{
    return QString::number(v);
}

inline QString str(char const *v)
{
    return QString(v);
}

inline bool is(QVariant const &v)
{
    return v.toBool();
}

inline bool is(QString const &v)
{
    return v.isEmpty() ? false : v.toUInt();
}

inline bool hasType(QVariant const &v, QMetaType::Type t)
{
    return static_cast<QMetaType::Type>(v.type()) == t;
}

typedef QVariantMap map_type;
typedef QMap<QString, QString> string_map_type;
typedef QList<QVariant> list_type;
typedef std::tuple<QString, QVariant> map_tuple_type;

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

template <typename X, typename Y>
inline QMap<X, Y> map(QList<std::tuple<X, Y> > const &src)
{
    QMap<X, Y> res;
    for (auto it = src.begin(); it != src.end(); ++it) {
        res.insert(std::get<0>(*it), std::get<1>(*it));
    }
    return res;
}

QVariant get(QVariantMap const &m, QString const &k1)
{
    return m[k1];
}

inline QStringList filterEmpty(QStringList const &src)
{
    return src.filter(QRegExp("^.+$"));
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
QList<ResT> map(FnT fn, QList<T> const &src)
{
    QList<ResT> res;
    for (auto it = src.begin(); it != src.end(); ++it) {
        res.push_back(fn(*it));
    }
    return res;
}

template <typename ResT, typename FnT, typename K, typename V>
QList<ResT> map(FnT fn, QMap<K, V> const &src)
{
    QList<ResT> res;
    for (auto it = src.begin(); it != src.end(); ++it) {
        res.push_back(fn(it.key(), it.value()));
    }
    return res;
}

template <typename ResT, typename FnT>
QList<ResT> map(FnT fn, QString const &src)
{
    QList<ResT> res;
    for (auto it = src.begin(); it != src.end(); ++it) {
        res.push_back(fn(*it));
    }
    return res;
}

template <typename X, typename Y>
QList<std::tuple<X, Y> > zip(QList<X> const &x, QList<Y> const &y)
{
    QList<std::tuple<X, Y> > res;
    auto yit = y.begin();
    auto xit = x.begin();
    while(xit != x.end()) {
        if (yit != y.end())
            res.push_back(std::make_tuple(*xit, *yit));
        else
            break;
    }
    while(xit != x.end())
        res.push_back(std::make_tuple(*xit, Y()));

    return res;
}

template <typename ResKeyT, typename T, typename K>
QMap<ResKeyT, T> mapByField(QList<T> const &src, K const &key)
{
    auto fn = [&key](T const &v) { return std::make_tuple(ResKeyT(v[key]), v); };
    auto pairs = map<std::tuple<ResKeyT, T> >(fn, src);
    return ::map(pairs);
}

double parseBytes(QString const &s, QString const &unit = "b"
                  , long multiplier = 1024);

}

#endif // _CUTES_UTIL_HPP_
