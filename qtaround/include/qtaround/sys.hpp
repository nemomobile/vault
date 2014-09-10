#ifndef _CUTES_SYS_HPP_
#define _CUTES_SYS_HPP_
/**
 * @file sys.hpp
 * @brief Command line parsing and generation etc.
 * @author Denis Zalevskiy <denis.zalevskiy@jolla.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <qtaround/util.hpp>

#include <QSet>

namespace sys {

QStringList command_line_options
(QVariantMap const &options
 , string_map_type const &short_options = string_map_type()
 , string_map_type const &long_options = string_map_type()
 , QSet<QString> const &options_has_param = QSet<QString>());

class GetOpt
{
public:
    virtual ~GetOpt() {}
    virtual QString value(QString const &name) const =0;
    virtual bool isSet(QString const &name) const =0;
    virtual QStringList arguments() const =0;
};

std::unique_ptr<GetOpt> getopt(QVariantMap const &, bool requireAll = true);

}

#endif // _CUTES_SYS_HPP_
