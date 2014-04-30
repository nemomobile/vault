#ifndef _CUTES_OS_HPP_
#define _CUTES_OS_HPP_

#include "error.hpp"
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
        return join(a+b, std::forward<A>(args)...);
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

private:

};

int system(QString const &cmd, QStringList const &args = QStringList());

bool mkdir(QString const &path, QVariantMap const &options);

static inline bool mkdir(QString const &path)
{
    return mkdir(path, QVariantMap());
}

static inline QByteArray read_file(QString const &fname)
{
    QFile f(fname);
    if (!f.open(QFile::ReadOnly))
        return QByteArray();
    return f.readAll();
}

static inline size_t write_file(QString const &fname, QByteArray const &data)
{
    QFile f(fname);
    if (!f.open(QFile::WriteOnly))
        return 0;
    
    return f.write(data);
}

static inline size_t write_file(QString const &fname, QString const &data)
{
    return write_file(fname, data.toUtf8());
}

}

#endif // _CUTES_OS_HPP_
