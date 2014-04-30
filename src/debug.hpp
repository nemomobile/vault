#ifndef _CUTES_DEBUG_HPP_
#define _CUTES_DEBUG_HPP_

#include <QVariant>
#include <QDebug>
#include <QTextStream>

#include <memory>

namespace debug {

enum class Level { Debug, Info, Warning, Error, Critical };

QDebug stream();

static inline void print(QDebug &&d)
{
    QDebug dd(std::move(d));
}

template <typename T, typename ... A>
void print(QDebug &&d, T &&v1, A&& ...args)
{
    d << v1;
    return print(std::move(d), std::forward<A>(args)...);
}

template <typename ... A>
void print(A&& ...args)
{
    return print(stream(), std::forward<A>(args)...);
}

void level(Level);
bool is_tracing_level(Level);

template <Level print_level, typename ... A>
void print_ge(A&& ...args)
{
    if (is_tracing_level(print_level))
        print(std::forward<A>(args)...);
}

}

#endif // _CUTES_DEBUG_HPP_
