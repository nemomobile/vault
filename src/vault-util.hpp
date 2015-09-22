#ifndef _VAULT_UTIL_HPP_
#define _VAULT_UTIL_HPP_

#include "git-util.hpp"
#include "common-util.hpp"

#define SHA1_HASH_SIZE (40)

size_t vault_uri_blob_prefix_size();

#define VAULT_URI_MAX_SIZE (vault_uri_blob_prefix_size() + SHA1_HASH_SIZE)

class Vault : public qtaround::git::Tree
{
public:
    Vault(QString const &path)
        : qtaround::git::Tree(find_root(path))
    {}

    QString root() const { return root_; }
    QString blobs() const;
    QString blob_path(QString const &) const;

    QString path_from_uri(QString const &);
    QString uri_from_hash(QString const &);
    QString blob_hash(QString const &path);
    bool is_blob_path(QFileInfo const &path);
    bool is_blob_path(QString const &path)
    {
        return is_blob_path(QFileInfo(path));
    }
private:
    QString find_root(QString const &path);
};

typedef std::shared_ptr<Vault> VaultHandle;

inline QDebug & operator << (QDebug &dst, Vault const &v)
{
    dst << "Vault[" << v.root() << "]";
    return dst;
}

inline QString Vault::blobs() const
{
    return storage("blobs");
}


#endif // _VAULT_UTIL_HPP_
