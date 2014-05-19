#ifndef _VAULT_TESTS_COMMON_HPP_
#define _VAULT_TESTS_COMMON_HPP_

#include <QStringList>

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

}

#endif // _VAULT_TESTS_COMMON_HPP_
