#ifndef _CUTES_UNIT_HPP_
#define _CUTES_UNIT_HPP_
/**
 * @file unit.hpp
 * @brief Vault unit option parsing and file export/import support
 * @author Denis Zalevskiy <denis.zalevskiy@jolla.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <vault/config.hpp>
#include <memory>

namespace sys { class GetOpt; }

namespace vault { namespace unit {

std::unique_ptr<sys::GetOpt> getopt();

int execute(std::unique_ptr<sys::GetOpt>, QVariantMap const &info);

}}

#endif // _CUTES_UNIT_HPP_
