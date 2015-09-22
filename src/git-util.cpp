#include "git-util.hpp"
#include "file-util.hpp"
#include "common-util.hpp"
#include <qtaround/subprocess.hpp>
#include <qtaround/error.hpp>
#include <qtaround/util.hpp>
#include <QVariantMap>
#include <QTemporaryFile>
#include <stdlib.h>

namespace subprocess = qtaround::subprocess;
namespace error = qtaround::error;

namespace qtaround { namespace git {

Tree::Tree(QString const &path)
    : root_(path)
{
    ps_.setWorkingDirectory(root_);
}

Tree::~Tree()
{
}

QString Tree::execute(QStringList const &params)
{
    auto out = ps_.check_output("git", {params});
    return out.trimmed();
}

QString Tree::resolve_storage(QString const &root)
{
    QString res;
    QFileInfo dotgit(path(root, ".git"));
    if (dotgit.isDir()) {
        res = dotgit.filePath();
    } else if (dotgit.isFile()) {
        auto data = read_text(dotgit.filePath(), 1024);
        static const QString prefix{"gitdir: "};
        if (data.mid(0, prefix.size()) != prefix)
            error::raise({{"msg", "Wrong .git data"}, {"data", data}});
        res = data.mid(prefix.size(), data.indexOf(" "));
    } else {
        error::raise({{"msg", "Unhandled .git type"}, {"type", str(dotgit)}});
    }
    return path_normalize(res);
}

QString Tree::storage() const
{
    if (storage_.isEmpty())
        storage_ = resolve_storage(root_);
    return storage_;
}

QString Tree::index_add
(QString const &hash ,QString const &name, mode_t mode)
{
    auto cacheinfo = join(",", QString::number(mode), hash, name);
    return execute("update-index", "--add", "--cacheinfo", cacheinfo);
}

size_t Tree::blob_add
(FileHandle const &src, size_t left_size, size_t max_chunk_size
 , QString const &entry_name)
{
    debug::debug("Adding blob from ", src->fileName(), " size ", left_size
                 , " as ", entry_name);
    size_t idx = 0;
    off_t off = 0;
    size_t size = left_size;
    auto dst = std::make_shared<QTemporaryFile>();
    if (!dst->open())
        raise_std_error({{"msg", "Can't open tmp file"}});

    auto dst_path = dst->fileName();
    if (left_size > max_chunk_size) {
        debug::debug("Split to chunks");
        dst->resize(max_chunk_size);
        while (left_size) {
            size = (left_size > max_chunk_size) ? max_chunk_size : left_size;
            if (size < max_chunk_size)
                dst->resize(size);
            copy(fileHandle(dst), src, size, off);
            left_size -= size;
            off += size;
            auto dst_hash = blob_add(dst_path);
            index_add(dst_hash, path(entry_name, QString::number(idx++)));
        }
    } else {
        dst->resize(size);
        copy(fileHandle(dst), src, size, off);
        auto dst_hash = blob_add(dst_path);
        index_add(dst_hash, entry_name);
    }
    unlink(dst_path);
    return idx;
}

}}
