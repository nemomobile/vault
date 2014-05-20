#ifndef _CUTES_DEBUG_HPP_
#define _CUTES_DEBUG_HPP_
/**
 * @file debug.hpp
 * @brief Debug tracing support
 * @author Denis Zalevskiy <denis.zalevskiy@jolla.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <QVariant>
#include <QDebug>
#include <QTextStream>

#include <memory>

namespace debug {

enum class Level { Debug = 1, Info, Warning, Error, Critical };

QDebug stream();

template <typename T, typename ... Args>
QDebug & operator << (QDebug &d, std::function<T(Args...)> const &)
{
    d << "std::function<...>";
    return d;
}

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

template <typename ... A>
void print_ge(Level print_level, A&& ...args)
{
    if (is_tracing_level(print_level))
        print(std::forward<A>(args)...);
}

template <typename ... A>
void debug(A&& ...args)
{
    print_ge(Level::Debug, std::forward<A>(args)...);
}

template <typename ... A>
void info(A&& ...args)
{
    print_ge(Level::Info, std::forward<A>(args)...);
}

template <typename ... A>
void warning(A&& ...args)
{
    print_ge(Level::Warning, std::forward<A>(args)...);
}

template <typename ... A>
void error(A&& ...args)
{
    print_ge(Level::Error, std::forward<A>(args)...);
}

template <typename ... A>
void critical(A&& ...args)
{
    print_ge(Level::Critical, std::forward<A>(args)...);
}

}

#endif // _CUTES_DEBUG_HPP_
