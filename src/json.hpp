#ifndef _CUTES_JSON_HPP_
#define _CUTES_JSON_HPP_

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include "os.hpp"

namespace json {

static inline QVariantMap read(QString const &fname)
{
    auto data = os::read_file(fname);
    auto doc = QJsonDocument::fromJson(data);
    auto obj = doc.object();
    QVariantMap res;
    return res;
}

}

#endif // _CUTES_JSON_HPP_
