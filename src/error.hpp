#ifndef _CUTES_ERROR_HPP_
#define _CUTES_ERROR_HPP_

#include <exception>
#include <QVariantMap>

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

template <typename T, typename T2, typename ... A>
void raise(T const &m1, T2 const &m2, A && ...args)
{
    QVariantMap x = m1;
    QVariantMap y = m2;
    x.unite(y);
    raise(x, std::forward<A>(args)...);
}

}

#endif // _CUTES_ERROR_HPP_
