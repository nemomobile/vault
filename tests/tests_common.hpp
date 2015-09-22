#ifndef _VAULT_TESTS_COMMON_HPP_
#define _VAULT_TESTS_COMMON_HPP_

#include <QStringList>
#include <qtaround/util.hpp>

namespace {

template <class CharT>
std::basic_ostream<CharT>& operator <<
(std::basic_ostream<CharT> &dst, QStringList const &src)
{
    for (auto v : src)
        dst  << v.toStdString() << ",";
    return dst;
}

template <class CharT, class T>
std::basic_ostream<CharT>& operator <<
(std::basic_ostream<CharT> &dst, QSet<T> const &src)
{
    for (auto v : src)
        dst  << str(v) << ",";
    return dst;
}

template <class CharT>
std::basic_ostream<CharT>& operator <<
(std::basic_ostream<CharT> &dst, QString const &src)
{
    dst << src.toStdString();
    return dst;
}

inline QStringList strings()
{
    return QStringList();
}

template <typename ...A>
QStringList strings(QString const &s, A &&...args)
{
    return QStringList(s) + strings(std::forward<A>(args)...);
}

}

#define S_(...) strings(__VA_ARGS__).join(' ').toStdString()

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__  ":" TOSTRING(__LINE__)

#endif // _VAULT_TESTS_COMMON_HPP_
