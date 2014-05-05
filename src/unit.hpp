#ifndef _CUTES_UNIT_HPP_
#define _CUTES_UNIT_HPP_

#include "util.hpp"
#include "os.hpp"
#include "vault_config.hpp"
#include "json.hpp"
#include "sys.hpp"

#include <cor/options.hpp>

namespace vault { namespace unit {

std::unique_ptr<sys::GetOpt> getopt();

int execute(std::unique_ptr<sys::GetOpt>, QVariantMap const &info);

}}

#endif // _CUTES_UNIT_HPP_
