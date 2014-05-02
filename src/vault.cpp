
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

bool Vault::init()
{
    if (!m_vcs.init()) {
        return false;
    }

    m_vcs.setConfigValue("status.showUntrackedFiles", "all");
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
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QTextStream stream(&file);
    stream << content << '\n';
    file.close();
    return true;
}

bool Vault::setState(const QString &state)
{
    return writeFile(".vault.state", state);
}
