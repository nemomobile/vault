#ifndef _CUTES_OS_HPP_
#define _CUTES_OS_HPP_

#include "error.hpp"
#include "sys.hpp"
#include "subprocess.hpp"

#include <QFileInfo>
#include <QDir>
#include <QString>
#include <QVariant>

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

namespace {

inline QByteArray read_file(QString const &fname)
{
    QFile f(fname);
    if (!f.open(QFile::ReadOnly))
        return QByteArray();
    return f.readAll();
}

inline ssize_t write_file(QString const &fname, QByteArray const &data)
{
    QFile f(fname);
    if (!f.open(QFile::WriteOnly))
        return 0;
    
    return f.write(data);
}

inline ssize_t write_file(QString const &fname, QString const &data)
{
    return write_file(fname, data.toUtf8());
}

inline ssize_t write_file(QString const &fname, char const *data)
{
    return write_file(fname, QString(data).toUtf8());
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

}

int cp(QString const &src, QString const &dst
       , QVariantMap &&options = QVariantMap());
int update(QString const &src, QString const &dst
           , QVariantMap &&options = QVariantMap());
int update_tree(QString const &src, QString const &dst
                , QVariantMap &&options = QVariantMap());
int cptree(QString const &src, QString const &dst
           , QVariantMap &&options = QVariantMap());

}

#endif // _CUTES_OS_HPP_
