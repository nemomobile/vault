#include "file-util.hpp"
#include <qtaround/error.hpp>
#include <qtaround/os.hpp>
#include <fstream>
#include <fcntl.h>
#include <sys/stat.h>

namespace error = qtaround::error;
namespace os = qtaround::os;

QString path_normalize(QString const &path)
{
    auto res = path.simplified();
    auto pos = res.size();
    while (--pos) {
        if (res[pos] != QChar('/'))
            break;
    }
    return (pos + 1 == res.size()) ? res : res.left(pos);
}

static int get_stat(QString const &path, struct stat &stat)
{
    auto data = path.toLocal8Bit();
    return ::lstat(data.data(), &stat);
}

static void get_stat_or_fail(QString const &path, struct stat &stat)
{
    int rc = get_stat(path, stat);
    if (rc < 0)
        raise_std_error({{"msg", "Can't get stat"}, {"path", path}});
}

bool is_older(QString const &path_a, QString const &path_b)
{
    struct stat stat_a;
    struct stat stat_b;
    get_stat_or_fail(path_a, stat_a);
    get_stat_or_fail(path_b, stat_b);
    return is_older(stat_a, stat_b);
}

static void get_file_times(QString const &path, struct timespec *times)
{
    struct stat stat;
    get_stat_or_fail(path, stat);
    ::memcpy(&times[0], &stat.st_atime, sizeof(times[0]));
    ::memcpy(&times[1], &stat.st_mtime, sizeof(times[1]));
}

FileId fileId(QString const &from)
{
    struct stat stat;
    get_stat_or_fail(from, stat);
    FileId res(stat);
    debug::debug("fileId for", from, "=", res);
    return std::move(res);
}

FileId fileId(QFileInfo const &from)
{
    if (!exists(from))
        error::raise({{"msg", "Can't create FileId"}, {"path", str(from)}});
    return fileId(from.filePath());
}

static void copy_utime(int fd, QFileInfo const &src)
{
    struct timespec times[2];
    get_file_times(src.filePath(), times);
    int rc = ::futimens(fd, times);
    if (rc < 0)
        raise_std_error({{"msg", "Can't change time"}, {"target", fd}});
}

void copy_utime(QFile const &fd, QFileInfo const &src)
{
    return copy_utime(fd.handle(), src);
}

void copy_utime(QString const &target, QFileInfo const &src)
{
    struct timespec times[2];
    get_file_times(src.filePath(), times);
    auto tgt_cs = target.toLocal8Bit();
    int rc = ::utimensat(AT_FDCWD, tgt_cs.data(), times, AT_SYMLINK_NOFOLLOW);
    if (rc < 0)
        raise_std_error({{"msg", "Can't change time"}, {"target", target}});
}

void mkdir(QString const &path, FileMode)
{
    // TODO use path
    QFileInfo info(path);
    auto parent = info.dir();
    if (parent.exists()) {
        parent.mkdir(info.fileName());
    }
}

QFileInfo mkdir_similar(QFileInfo const &from
                        , QFileInfo const &parent
                        , CopyTime copy_time)
{
    if (!parent.exists())
        error::raise({{"msg", "No parent dir"}, {"parent", str(parent)}});

    auto dst_path = path(parent.filePath(), from.fileName());
    debug::debug("mkdir", dst_path, " similar to ", str(from));
    QFileInfo dst_stat(dst_path);
    if (!exists(dst_stat)) {
        mkdir(dst_path, from.permissions());
        if (copy_time == CopyTime::NewOnly)
            copy_utime(dst_path, from);
        dst_stat.refresh();
    } else if (dst_stat.isDir()) {
        debug::debug("Already exists", dst_path);
        if (copy_time == CopyTime::Always)
            copy_utime(dst_path, from);
    } else {
        error::raise({{"msg", "Destination type is different"}
                , {"src", str(from)}, {"parent", str(parent)}
                , {"dst", str(dst_stat)}});
    }
    return std::move(dst_stat);
}

void unlink(QString const &path)
{
    auto data = path.toLocal8Bit();
    auto rc = ::unlink(data.data());
    if (rc < 0)
        raise_std_error({{"msg", "Can't unlink"}, {"path", path}});
}

void copy(FileHandle const &dst, FileHandle const &src, size_t size, off_t off)
{
    if (!size)
        return;
    auto p_src = mmap_create(nullptr, size, PROT_READ
                                 , MAP_PRIVATE, src->handle(), off);
    auto p_dst = mmap_create(nullptr, size, PROT_READ | PROT_WRITE
                             , MAP_SHARED, dst->handle(), off);
    memcpy(mmap_ptr(p_dst), mmap_ptr(p_src), size);
}

void copy(FileHandle &dst, FileHandle &src, size_t left_size
          , ErrorCallback on_error)
{
    int rc = ::ftruncate(dst->handle(), left_size);
    if (rc < 0)
        on_error({{"msg", "Can't resize"}, {"size", (unsigned)left_size}});

    auto off = ::lseek(dst->handle(), left_size, SEEK_SET);
    if (off < 0)
        on_error({{"msg", "Can't seek"}, {"size", (unsigned)left_size}});

    // goto beginning
    off = ::lseek(src->handle(), 0, SEEK_SET);
    if (off < 0)
        on_error({{"msg", "Can't goto beginning"}});

    static size_t max_chunk_size = 1024 * 1024;
    size_t size = left_size;

    while (left_size) {
        auto prev_off = off;
        off = ::lseek(src->handle(), prev_off, SEEK_DATA);
        if (off < 0)
            on_error({{"msg", "Can't find data"}});

        auto delta = off - prev_off;
        if ((size_t)delta > left_size)
            return;
        left_size -= delta;
        auto off_hole = ::lseek(src->handle(), off, SEEK_HOLE);
        if (off_hole < 0)
            on_error({{"msg", "Can't find hole"}, {"off", (unsigned)prev_off}});
        delta = off_hole - off;
        size = ((size_t)delta < max_chunk_size
                ? delta
                : (left_size > max_chunk_size
                   ? max_chunk_size
                   : left_size));
        copy(dst, src, size, off);
        left_size -= size;
        off += size;
    }
}

mode_t toPosixMode(QFileDevice::Permissions perm)
{
    return (perm & QFileDevice::ReadOwner? 0600 : 0)
        | (perm & QFileDevice::WriteOwner? 0400 : 0)
        | (perm & QFileDevice::ExeOwner? 0200 : 0)
        | (perm & QFileDevice::ReadGroup? 060 : 0)
        | (perm & QFileDevice::WriteGroup? 040 : 0)
        | (perm & QFileDevice::ExeGroup? 020 : 0)
        | (perm & QFileDevice::ReadOther? 06 : 0)
        | (perm & QFileDevice::WriteOther? 04 : 0)
        | (perm & QFileDevice::ExeOther? 02 : 0);
}

inline int open_qs(QString const &name, int flags, mode_t mode)
{
    auto data = name.toLocal8Bit();
    return ::open(data.data(), flags, mode);
}

FileHandle open_file(QString const &path, int flags, FileMode *pmode)
{
    auto handle = std::make_shared<QFile>(path);
    auto fd = (pmode
               ? open_qs(path, flags, toPosixMode(*pmode))
               : open_qs(path, flags, 0640));
    if (fd >= 0)
        handle->open(fd, QIODevice::ReadWrite, QFileDevice::AutoCloseHandle);

    return handle;
}

FileHandle copy_data(QString const &dst_path
                     , QFileInfo const &from, FileMode *pmode)
{
    debug::debug("Copy file data: ", str(from), "->", dst_path
                 , "mode", (pmode ? *pmode : -1));
    auto src = std::make_shared<QFile>(from.filePath());
    if (!src->open(QIODevice::ReadOnly))
        error::raise({{"msg", "Cant' open src file"}, {"stat", str(from)}});
    auto raise_dst_error = [&dst_path](QVariantMap const &info) {
        error::raise(map({{"dst", dst_path}
                    , {"error", ::strerror(errno)}}), info);
    };
    auto dst = open_file(dst_path, O_RDWR | O_CREAT, pmode);
    if (!dst->isOpen())
        raise_dst_error({{"msg", "Cant' open dst file"}});
    using namespace std::placeholders;
    copy(dst, src, from.size(), std::bind(raise_dst_error, _1));
    return std::move(dst);
}


FileHandle rewrite(QString const &dst_path
                   , QString const &text
                   , FileMode mode)
{
    auto flags = O_CREAT | O_TRUNC | O_WRONLY;
    auto fd = ::open(dst_path.toLocal8Bit(), flags, toPosixMode(mode));
    auto dst = std::make_shared<QFile>(dst_path);
    if (!dst->open(fd, QIODevice::WriteOnly, QFileDevice::AutoCloseHandle))
        error::raise({{"msg", "Can't open dst"}, {"path", dst_path}});
    auto written = dst->write(text.toLocal8Bit(), text.size());
    if (written != (long)text.size())
        error::raise({{"msg", "Error writing"}
                , {"error", ::strerror(errno)}
                , {"path", dst_path}
                , {"data", text}
                , {"res", written}});
    return std::move(dst);
}

QString read_text(QString const &src_path, long long max_size)
{
    QFile f(src_path);
    if (!f.open(QIODevice::ReadOnly))
        error::raise({{"msg", "Can't open"}, {"path", src_path}});
    return QString::fromUtf8(f.read(max_size));
}

void symlink(QString const &tgt, QString const &link)
{
    auto tgt_cs = tgt.toLocal8Bit();
    auto link_cs = link.toLocal8Bit();
    auto rc = ::symlink(tgt_cs.data(), link_cs.data());
    if (rc < 0)
        raise_std_error({{"msg", "Can't create link"}
                , {"tgt", tgt}, {"link", link}});
}

QString readlink(QString const &path)
{
    auto path_cs = path.toLocal8Bit();
    char buf[PATH_MAX];
    auto rc = ::readlink(path_cs, buf, sizeof(buf));
    if (rc < 0)
        raise_std_error({{"msg", "Can't read link"}, {"link", path}});
    if (rc == sizeof(buf))
        error::raise({{"msg", "Small buf reading link"}, {"path", path}});
    buf[rc] = '\0';
    return QString::fromLocal8Bit(buf);
}

bool exists(QFileInfo const &path)
{
    struct stat stat;
    int rc = get_stat(path.filePath(), stat);
    bool res;
    if (rc < 0) {
        int err = errno;
        if (err == ENOENT || err == ENOTDIR)
            res = false;
        else
            raise_std_error({{"msg", "Failed to get stat"}, {"path", str(path)}});
    } else {
        res = true;
    }
    return res;
}
