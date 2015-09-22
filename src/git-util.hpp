#ifndef _COR_GIT_UTIL_HPP_
#define _COR_GIT_UTIL_HPP_

#include "common-util.hpp"
#include "file-util.hpp"
#include <cor/util.hpp>
#include <qtaround/subprocess.hpp>
#include <qtaround/util.hpp>

namespace qtaround {
namespace git {

template <typename ... Args>
QString join(QString const &delim, Args&& ...args)
{
    return QStringList({args...}).join(delim);
}

class Tree {
public:
    Tree(QString const &path);
    virtual ~Tree();
    QString storage() const;

    template <typename ... Args>
    QString storage(QString const &arg1, Args&& ...args) const
    {
        return path(storage(), arg1, std::forward<Args>(args)...);
    }

    QString execute(QStringList const &params);

    template <typename ... Args>
    QString execute(QString const &arg1, Args&& ...args)
    {
        return execute({arg1, std::forward<Args>(args)...});
    }

    QString hash_file(QString const &path)
    {
        return execute("hash-object", path);
    }

    QString blob_add(QString const &path)
    {
        return execute("hash-object", "-w", "-t", "blob", path);
    }

    QString index_add(QString const &hash
                      , QString const &name
                      , mode_t mode = 0100644);

    size_t blob_add(FileHandle const &src, size_t left_size
                    , size_t max_chunk_size, QString const &entry_name);
    
protected:
    static QString resolve_storage(QString const &root);

    QString root_;
    qtaround::subprocess::Process ps_;
    mutable QString storage_;
};


}
}

#endif // _COR_GIT_UTIL_HPP_
