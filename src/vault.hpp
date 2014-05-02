
#ifndef VAULT_H
#define VAULT_H

#include <QString>

#include <libgit/repo.h>

class Vault
{
public:
    Vault(const QString &path);

    bool init();

    bool writeFile(const QString &file, const QString &content);

private:
    bool setState(const QString &state);

    QString m_path;
    LibGit::Repo m_vcs;
};

#endif
