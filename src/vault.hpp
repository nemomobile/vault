
#ifndef VAULT_H
#define VAULT_H

#include <functional>

#include <QString>

#include <libgit/repo.h>

class Vault
{
public:
    struct BackupResult {
        QStringList succededUnits;
        QStringList failedUnits;
    };

    typedef std::function<void (const QString &, const QString &)> ProgressCallback;
    Vault(const QString &path);

    bool init(const QVariantMap &config = QVariantMap());
    BackupResult backup(const QString &home, const QStringList &units, const QString &message, const ProgressCallback &callback = nullptr);

    bool writeFile(const QString &file, const QString &content);

private:
    bool setState(const QString &state);
    bool backupUnit(const QString &unit, const ProgressCallback &callback);
    void tagSnapshot(const QString &msg);

    QString m_path;
    LibGit::Repo m_vcs;
};

#endif
