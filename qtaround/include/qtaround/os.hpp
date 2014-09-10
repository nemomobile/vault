#ifndef _CUTES_OS_HPP_
#define _CUTES_OS_HPP_
/**
 * @file os.hpp
 * @brief Wrappers to support API looking familiar for python/shell developers
 * @author Denis Zalevskiy <denis.zalevskiy@jolla.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include "error.hpp"
#include "sys.hpp"
#include "subprocess.hpp"

#include <QFileInfo>
#include <QDir>
#include <QString>
#include <QVariant>
#include <QDateTime>

using subprocess::Process;

namespace os {
class path
{
public:
    static inline QString join(QString const& a)
    {
        return a;
    }

    template <typename ...A>
    static QString join(QString const& a, QString const& b, A&& ...args)
    {
        return join(QStringList({a, b}).join("/"), std::forward<A>(args)...);
    }

    static inline bool exists(QString const &p)
    {
        return QFileInfo(p).exists();
    }
    static inline QString canonical(QString const &p)
    {
        return QFileInfo(p).canonicalFilePath();
    }
    static inline QString relative(QString const &p, QString const &d)
    {
        return QDir(d).relativeFilePath(p);
    }
    static inline QString deref(QString const &p)
    {
        return QFileInfo(p).symLinkTarget();
    }
    static QString target(QString const &link);

    static inline bool isDir(QString const &p)
    {
        return QFileInfo(p).isDir();
    }
    static inline bool isSymLink(QString const &p)
    {
        return QFileInfo(p).isSymLink();
    }
    static inline bool isFile(QString const &p)
    {
        return QFileInfo(p).isFile();
    }
    static inline QString dirName(QString const &p)
    {
        return QFileInfo(p).dir().path();
    }
    static inline QString fileName(QString const &p)
    {
        return QFileInfo(p).fileName();
    }

    static QStringList split(QString const &p);

    static bool isDescendent(QString const &p, QString const &other);

private:

};

int system(QString const &cmd, QStringList const &args = QStringList());

bool mkdir(QString const &path, QVariantMap const &options);

static inline bool mkdir(QString const &path)
{
    return mkdir(path, QVariantMap());
}

QByteArray read_file(QString const &fname);
ssize_t write_file(QString const &fname, QByteArray const &data);

static inline ssize_t write_file(QString const &fname, QString const &data)
{
    return write_file(fname, data.toUtf8());
}

static inline ssize_t write_file(QString const &fname, char const *data)
{
    return write_file(fname, QString(data).toUtf8());
}

namespace {

inline QString home()
{
    return QDir::homePath();
}

inline int symlink(QString const &tgt, QString const &link)
{
    return system("ln", {"-s", tgt, link});
}

inline int rmtree(QString const &path)
{
    return system("rm", {"-rf", path});
}

inline int unlink(QString const &what)
{
    return system("unlink", {what});
}

inline int rename(QString const &from, QString const &to)
{
    return system("mv", {from, to});
}

inline QDateTime lastModified(QString const &path)
{
    return QFileInfo(path).lastModified();
}

inline int setLastModified(QString const &path, QDateTime const &timeval)
{
    return system("touch", {"-d", timeval.toString(), path});
};

inline int rm(QString const &path)
{
    return system("rm", {path});
}

inline QString environ(QString const &name)
{
    auto c = ::getenv(name.toUtf8());
    return c ? str(c) : QString();
}

}

int cp(QString const &, QString const &, QVariantMap &&);

static inline int cp(QString const &src, QString const &dst)
{
    return cp(src, dst, QVariantMap());
}

int update(QString const &src, QString const &dst
           , QVariantMap &&options = QVariantMap());

int update_tree(QString const &, QString const &, QVariantMap &&);

static inline int update_tree(QString const &src, QString const &dst)
{
    return update_tree(src, dst, QVariantMap());
}

int cptree(QString const &src, QString const &dst
           , QVariantMap &&options = QVariantMap());

size_t get_block_size(QString const &);

QList<QVariantMap> mount();
QString mountpoint(QString const &path);
string_map_type stat(QString const &path, QVariantMap &&options = QVariantMap());
QVariant du(QString const &path, QVariantMap &&options = map({{"summarize", true}
            , {"one_filesystem", true}, {"block_size", "K"}}));
double diskFree(QString const &path);
QString mkTemp(QVariantMap &&options = QVariantMap());

}

#endif // _CUTES_OS_HPP_
