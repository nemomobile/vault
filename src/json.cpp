#include "json.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

namespace json {

static QVariant array2VariantMap(QJsonArray const &a);
static QVariantMap object2VariantMap(QJsonObject const &o);

static QVariant value2Variant(QJsonValue const &v)
{
    if (v.isObject()) {
        return object2VariantMap(v.toObject());
    } else if (v.isArray()) {
        return array2VariantMap(v.toArray());
    }
    return v.toVariant();
}

static QVariant array2VariantMap(QJsonArray const &a)
{
    size_t i = 0;
    QVariantList res;
    for (auto it = a.begin(); it != a.end(); ++it, ++i)
        res.push_back(value2Variant(*it));
    return res;
}

static QVariantMap object2VariantMap(QJsonObject const &o)
{
    QVariantMap res;
    for (auto it = o.begin(); it != o.end(); ++it)
        res.insert(it.key(), value2Variant(it.value()));
    return res;
}

QVariantMap read(QString const &fname)
{
    auto data = os::read_file(fname);
    auto doc = QJsonDocument::fromJson(data);
    return object2VariantMap(doc.object());
}

}
