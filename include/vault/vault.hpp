#ifndef _VAULT_VAULT_HPP_
#define _VAULT_VAULT_HPP_
/**
 * @file vault.hpp
 * @brief Vault management API
 * @author Giulio Camuffo <giulio.camuffo@jollamobile.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <functional>

#include <QString>

#include <gittin/repo.hpp>

#include <vault/config.hpp>

namespace vault {

enum class File { Message, VersionTree, VersionRepo, State };

QString fileName(File);

class Snapshot
{
public:
    explicit Snapshot(const Gittin::Tag &commit);

    inline Gittin::Tag tag() const { return m_tag; }

    QString name() const;
    void remove();

private:
    Gittin::Tag m_tag;
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
    Result restore(const QString &snapshot, const QString &home, const QStringList &units, const ProgressCallback &callback = nullptr);
    bool clear(const QVariantMap &options);

    QList<Snapshot> snapshots() const;
    Snapshot snapshot(const QByteArray &tag) const;
    QString notes(const QString &snapshotName);

    bool exists() const;
    bool isInvalid();
    config::Vault config();
    UnitPath unitPath(const QString &name) const;
    inline QString root() const { return m_path; }

    void registerConfig(const QVariantMap &config);
    void unregisterUnit(const QString &unit);

    bool writeFile(const QString &file, const QString &content);

    static void execute(const QVariantMap &options);
    bool ensureValid();
    void reset(const QByteArray &treeish = QByteArray());

private:
    bool setState(const QString &state);
    bool backupUnit(const QString &home, const QString &unit, const ProgressCallback &callback);
    bool restoreUnit(const QString &home, const QString &unit, const ProgressCallback &callback);
    void tagSnapshot(const QString &msg);
    void resetMaster();

    void init_(const QVariantMap &config);

    QString readFile(const QString &relPath);

    void setVersion(File, int);
    int getVersion(File);

    const QString m_path;
    const QString m_blobStorage;
    Gittin::Repo m_vcs;
    config::Vault m_config;
};

}

#endif // _VAULT_VAULT_HPP_
