#ifndef _CUTES_ERROR_HPP_
#define _CUTES_ERROR_HPP_

#include "util.hpp"

namespace error {

class Error : public std::exception
{
public:
    Error(QVariantMap const &from) : m(from) {}
    virtual ~Error() noexcept(true) {};
    QVariantMap m;
};

static inline void raise(QVariantMap const &m)
{
    throw Error(m);
}

template <typename ... A>
void raise(QVariantMap const &m1, QVariantMap const &m2, A && ...args)
{
    QVariantMap m(m1);
    insert(m, m2);
    raise(m);
}

}

#endif // _CUTES_ERROR_HPP_
