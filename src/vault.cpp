
#include <QFile>
#include <QTextStream>

#include <libgit/commit.h>

#include "vault.hpp"
#include "os.hpp"

static const struct {
    int tree;
    int repository;
} version = { 2, 2 };

Vault::Vault(const QString &path)
     : m_path(path)
     , m_vcs(path)
{
}

bool Vault::init(const QVariantMap &config)
{
    if (!m_vcs.init()) {
        return false;
    }

    m_vcs.setConfigValue("status.showUntrackedFiles", "all");
    for (auto it = config.begin(); it != config.end(); ++it) {
        m_vcs.setConfigValue(it.key(), it.value().toString());
    }

    if (!writeFile(".git/info/exclude", ".vault.*")) {
        return false;
    }
    if (!writeFile(".vault", QString::number(version.tree))) {
        return false;
    }
    m_vcs.add(".vault");
    m_vcs.commit("anchor");
    m_vcs.tag("anchor");

    if (!os::path::exists(m_path + "/.git/blobs")) {
        os::mkdir(m_path + "/.git/blobs");
    }
    if (!writeFile(".git/vault.version", QString::number(version.repository))) {
        return false;
    }

    return setState("new");
}

bool Vault::writeFile(const QString &path, const QString &content)
{
    QFile file(m_path + "/" + path);
    if (!content.endsWith('\n')) {
        return os::write_file(m_path + "/" + path, content + '\n');
    }
    return os::write_file(m_path + "/" + path, content);
}

bool Vault::setState(const QString &state)
{
    return writeFile(".vault.state", state);
}
