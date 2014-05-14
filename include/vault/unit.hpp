#ifndef _CUTES_UNIT_HPP_
#define _CUTES_UNIT_HPP_

#include <vault/config.hpp>
#include <memory>

namespace sys { class GetOpt; }

namespace vault { namespace unit {

std::unique_ptr<sys::GetOpt> getopt();

int execute(std::unique_ptr<sys::GetOpt>, QVariantMap const &info);

}}

#endif // _CUTES_UNIT_HPP_
