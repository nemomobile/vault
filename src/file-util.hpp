#ifndef _FILE_UTIL_HPP_
#define _FILE_UTIL_HPP_

#include "common-util.hpp"
#include <qtaround/os.hpp>
#include <string>
#include <dirent.h>
#include <sys/stat.h>
#include <libgen.h>

namespace qtaround { namespace os {

namespace path {

inline QString dirName(QFileInfo const &info)
{
    return info.dir().path();
}

}

}}

typedef std::shared_ptr<QFile> FileHandle;

class FileId
{
public:
    FileId(struct stat const &src)
        : st_dev(src.st_dev), st_ino(src.st_ino)
    {}

    dev_t st_dev;
    ino_t st_ino;
};

FileId fileId(QFileInfo const &from);
FileId fileId(QString const &from);

inline bool operator == (FileId const &a, FileId const &b)
{
    return (a.st_dev == b.st_dev) && (a.st_ino == b.st_ino);
}

inline bool operator < (FileId const &a, FileId const &b)
{
    return (a.st_dev < b.st_dev) || (a.st_ino < b.st_ino);
}

QString path_normalize(QString const &);

inline QString str(QFileInfo const &info)
{
    return info.filePath();
}

QString readlink(QString const &path);
bool exists(QFileInfo const &path);

namespace {

inline QFileInfo readlink(QFileInfo const &from)
{
    return QFileInfo(readlink(from.filePath()));
}

inline QString path(std::initializer_list<QString> parts)
{
    return QStringList(parts).join("/");
}

template <typename ... Args>
QString path(QString v, Args&& ...args)
{
    return path({v, args...});
}

template <typename ... Args>
QString path(QFileInfo const &root, Args&& ...args)
{
    return path({root.filePath(), args...});
}

}

inline bool operator == (QFileInfo const &a, QFileInfo const &b)
{
    return a.exists() && b.exists() && fileId(a) == fileId(b);
}

class AFileTime
{
public:
    AFileTime(struct timespec const &src)
        : data_(src)
    {}
    virtual ~AFileTime() {}
    struct timespec const &raw() const {
        return data_;
    }
private:
    struct timespec data_;
};

enum class FileTimes { First_ = 0, Access = First_
        , Modification, Change, Last_ = Change };

template <FileTimes T>
class FileTime {};

template <>
class FileTime<FileTimes::Access> : public AFileTime
{
public:
    FileTime(struct stat const &src) : AFileTime(src.st_atim) {}
};

template <>
class FileTime<FileTimes::Modification> : public AFileTime
{
public:
    FileTime(struct stat const &src) : AFileTime(src.st_mtim) {}
};

template <>
class FileTime<FileTimes::Change> : public AFileTime
{
public:
    FileTime(struct stat const &src) : AFileTime(src.st_ctim) {}
};

inline bool operator == (AFileTime const &a, AFileTime const &b)
{
    return a.raw() == b.raw();
}

inline bool operator < (AFileTime const &a, AFileTime const &b)
{
    auto const &x = a.raw();
    auto const &y = b.raw();
    return (x.tv_sec < y.tv_sec) || (x.tv_sec == y.tv_sec
                                     && x.tv_nsec < y.tv_nsec);
}

inline bool is_older(struct stat const &a, struct stat const &b)
{
    return FileTime<FileTimes::Modification>(a)
        < FileTime<FileTimes::Modification>(b);
}

bool is_older(QString const &path_a, QString const &path_b);

struct MMap {
    MMap(void *pp, size_t l) : p(pp), len(l) { }
    void *p;
    size_t len;
};

struct MMapTraits
{
    typedef MMap handle_type;
    void close_(handle_type v) { ::munmap(v.p, v.len); }
    bool is_valid_(handle_type v) const { return v.p != nullptr; }
    handle_type invalid_() const { return MMap(nullptr, 0); }
};

typedef cor::Handle<MMapTraits> MMapHandle;
namespace {

inline MMapHandle mmap_create
(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    MMapHandle res(MMap(::mmap(addr, length, prot, flags, fd, offset), length)
                   , cor::only_valid_handle);
    return std::move(res);
}

inline void *mmap_ptr(MMapHandle const &p)
{
    return p.cref().p;
}

}

typedef QFile::Permissions FileMode;
namespace permissions {
static const FileMode dirForUserOnly =
    (QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
}

mode_t toPosixMode(QFileDevice::Permissions perm);

FileHandle open_file(QString const &path, int flags, FileMode *pmode);
void copy_utime(QFile const &fd, QFileInfo const &src);
void copy_utime(QString const &target, QFileInfo const &src);
enum class CopyTime { Never, Always, NewOnly };
QFileInfo mkdir_similar(QFileInfo const &from
                        , QFileInfo const &parent
                        , CopyTime copy_time);
void unlink(QString const &path);
void copy(FileHandle &dst, FileHandle &src, size_t left_size
          , ErrorCallback on_error);
FileHandle copy_data(QString const &dst_path
                     , QFileInfo const &from, FileMode *pmode);
FileHandle rewrite(QString const &dst_path
                   , QString const &text
                   , FileMode mode);
QString read_text(QString const &src_path, long long max_acceptable_size);
void mkdir(QString const &path, FileMode mode);

void copy(FileHandle const &dst, FileHandle const &src, size_t size, off_t off);

template <typename T>
FileHandle fileHandle(std::shared_ptr<T> const &from)
{
    return std::static_pointer_cast<QFile>(from);
}

void symlink(QString const &tgt, QString const &link);

// -----------------------------------------------------------------------------


inline QDebug & operator << (QDebug &d, FileId const &s)
{
    d << "(Node: " << s.st_dev << ", " << s.st_ino << ")";
    return d;
}

template <typename T1, typename T2>
QDebug & operator << (QDebug &d, std::pair<T1, T2> const &s)
{
    d << "(" << s.first << ", " << s.second << ")";
    return d;
}

// inline QString get_fname(int fd)
// {
//     auto fd_path = path("/proc/self/fd", std::to_string(fd));
//     return read_text(fd_path);
// }

// struct CFileTraits
// {
//     typedef FILE * handle_type;
//     void close_(handle_type v) { ::fclose(v); }
//     bool is_valid_(handle_type v) const { return v != nullptr; }
//     FILE* invalid_() const { return nullptr; }
// };

// typedef cor::Handle<CFileTraits> CFileHandle;

#endif // _FILE_UTIL_HPP_
