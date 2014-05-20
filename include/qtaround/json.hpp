#ifndef _CUTES_JSON_HPP_
#define _CUTES_JSON_HPP_
/**
 * @file json.hpp
 * @brief Json access wrappers
 * @author Denis Zalevskiy <denis.zalevskiy@jolla.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <qtaround/os.hpp>

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

namespace json {

QJsonObject read(QString const &);
ssize_t write(QJsonObject const &, QString const &);
ssize_t write(QVariantMap const &, QString const &);

}

namespace {

QVariant get(QJsonValue const &v)
{
    return v.toVariant();
}

QVariant get(QJsonObject const &m, QString const &k)
{
    return m[k].toVariant();
}

QVariant get(QJsonArray const &a, size_t i)
{
    return a[i].toVariant();
}

}

template <typename ... A>
QVariant get(QJsonValue const &v, QString const &k, A&& ...args)
{
    if (!v.isObject())
        return QVariant();
    return get(v.toObject()[k], std::forward<A>(args)...);
}

template <typename ... A>
QVariant get(QJsonValue const &v, size_t i, A&& ...args)
{
    if (!v.isArray())
        return QVariant();
    auto const a = v.toArray();
    return get(a[i], std::forward<A>(args)...);
}

template <typename ... A>
QVariant get(QJsonObject const &m, QString const &k, A&& ...args)
{
    auto v = m[k];
    return get(v, std::forward<A>(args)...);
}

#endif // _CUTES_JSON_HPP_
