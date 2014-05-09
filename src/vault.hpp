
#ifndef VAULT_H
#define VAULT_H

#include <functional>

#include <QString>

#include <libgit/repo.hpp>

#include <vault_config.hpp>

namespace vault {

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
    struct UnitPath {
        QString path;
        QString bin;
        QString data;
        bool exists() const;
    };

    typedef std::function<void (const QString &, const QString &)> ProgressCallback;
    Vault(const QString &path);

    bool init(const QVariantMap &config = QVariantMap());
    Result backup(const QString &home, const QStringList &units, const QString &message, const ProgressCallback &callback = nullptr);
    Result restore(const Snapshot &snapshot, const QString &home, const QStringList &units, const ProgressCallback &callback = nullptr);
    bool clear(const QVariantMap &options);

    QList<Snapshot> snapshots() const;
    Snapshot snapshot(const QByteArray &tag) const;

    bool exists() const;
    bool isInvalid();
    config::Vault config();
    UnitPath unitPath(const QString &name) const;
    inline QString root() const { return m_path; }

    void registerConfig(const QVariantMap &config);
    void unregisterUnit(const QString &unit);

    bool writeFile(const QString &file, const QString &content);

    static void execute(const QVariantMap &options);

private:
    bool setState(const QString &state);
    bool backupUnit(const QString &home, const QString &unit, const ProgressCallback &callback);
    bool restoreUnit(const QString &unit, const ProgressCallback &callback);
    void tagSnapshot(const QString &msg);
    void reset(const QByteArray &treeish = QByteArray());
    void resetMaster();

    const QString m_path;
    const QString m_blobStorage;
    LibGit::Repo m_vcs;
    config::Vault m_config;
};

}

#endif
