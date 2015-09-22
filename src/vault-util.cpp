#include "config.hpp"
#include "vault-util.hpp"

#include <qtaround/os.hpp>
#include <qtaround/subprocess.hpp>

#include <QString>

namespace os = qtaround::os;
namespace error = qtaround::error;
namespace subprocess = qtaround::subprocess;

#define QS_(s) QLatin1String(s)

static QString const uri_blob_prefix(QS_("vault://blob/"));

size_t vault_uri_blob_prefix_size() {
    return uri_blob_prefix.size();
}

QString Vault::find_root(QString const &path)
{
    subprocess::Process ps;
    QFileInfo info(path);
    auto wd = info.isDir() ? path : info.path();
    debug::debug(QS_("find root for"), wd);
    ps.setWorkingDirectory(wd);
    return ps.check_output(VAULT_LIBEXEC_PATH "/git-vault-root", {}).trimmed();
}

QString Vault::blob_path(QString const &hash) const
{
    if (hash.size() != SHA1_HASH_SIZE)
        error::raise({{QS_("msg"), QS_("Wrong hash")}, {QS_("hash"), hash}});

    return path(blobs(), hash.mid(0, 2), hash.mid(2));
}

QString Vault::path_from_uri(QString const &uri)
{
    if (uri.left(uri_blob_prefix.size()) != uri_blob_prefix)
        return QS_("");
    return blob_path(uri.mid(uri_blob_prefix.size()));
}

QString Vault::uri_from_hash(QString const &hash)
{
    if (hash.size() != SHA1_HASH_SIZE)
        error::raise({{QS_("msg"), QS_("Wrong hash")}, {QS_("hash"), hash}});

    return uri_blob_prefix + hash;
}

QString Vault::blob_hash(QString const &path)
{
    QFileInfo info(path);
    auto tail = info.fileName();
    auto head = info.dir().dirName();
    auto res = head + tail;
    if (res.size() != SHA1_HASH_SIZE)
        error::raise({{QS_("msg"), QS_("Wrong blob path")}, {QS_("path"), path}});
    return res;
}

bool Vault::is_blob_path(QFileInfo const &path)
{
    auto full_path = path.canonicalFilePath();
    return full_path.size() && os::path::isDescendent(full_path, blobs());
}
