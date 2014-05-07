
#ifndef VAULT_H
#define VAULT_H

#include <functional>

#include <QString>

#include <libgit/repo.h>

#include <vault_config.hpp>

class Snapshot
{
public:
    Snapshot(const LibGit::Tag &commit);

    inline LibGit::Tag tag() const { return m_tag; }

private:
    LibGit::Tag m_tag;
};

class Vault
{
public:
    struct Result {
        QStringList succededUnits;
        QStringList failedUnits;
    };

    typedef std::function<void (const QString &, const QString &)> ProgressCallback;
    Vault(const QString &path);

    bool init(const QVariantMap &config = QVariantMap());
    Result backup(const QString &home, const QStringList &units, const QString &message, const ProgressCallback &callback = nullptr);
    Result restore(const Snapshot &snapshot, const QString &home, const QStringList &units, const ProgressCallback &callback = nullptr);

    QList<Snapshot> snapshots() const;

    bool writeFile(const QString &file, const QString &content);

private:
    bool setState(const QString &state);
    bool backupUnit(const QString &home, const QString &unit, const ProgressCallback &callback);
    bool restoreUnit(const QString &unit, const ProgressCallback &callback);
    void tagSnapshot(const QString &msg);

    QString m_path;
    LibGit::Repo m_vcs;
    vault::config::Vault m_config;
};

#endif
