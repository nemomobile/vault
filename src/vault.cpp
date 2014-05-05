
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDateTime>

#include <libgit/commit.h>
#include <libgit/branch.h>

#include "vault.hpp"
#include "os.hpp"
#include "error.hpp"

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

Vault::Result Vault::backup(const QString &home, const QStringList &units, const QString &message, const ProgressCallback &callback)
{
    Result res;
    res.failedUnits << units;

    if (!os::path::isDir(home)) {
        qWarning("Home is not a dir: %s", qPrintable(home));
        return res;
    }

    ProgressCallback progress = callback ? callback : [](const QString &name, const QString &status) {
        qDebug() << "Progress" << name << status;
    };

    m_vcs.clean(LibGit::CleanOptions::Force | LibGit::CleanOptions::RemoveDirectories);
    m_vcs.reset(LibGit::ResetOptions::Hard);
    m_vcs.checkout("master", LibGit::CheckoutOptions::Force);

    if (!units.isEmpty()) {
        for (const QString &unit: units) {
            if (backupUnit(unit, progress)) {
                res.failedUnits.removeOne(unit);
                res.succededUnits << unit;
            }
        }
    } else {
        //TODO
    }

    QString timeTag = QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH-mm-ss.zzzZ");
    qDebug()<<timeTag<<message;
    QString msg = message.isEmpty() ? timeTag : message + '\n' + timeTag;
    writeFile(".message", msg);
    m_vcs.add(".message");
    LibGit::Commit commit = m_vcs.commit(timeTag + '\n' + msg);
    commit.addNote(message);
    tagSnapshot(timeTag);

    return res;
}

Vault::Result Vault::restore(const QString &home, const QStringList &units, const ProgressCallback &callback)
{
    Result res;
    res.failedUnits << units;

    if (!os::path::isDir(home)) {
        qWarning("Home is not a dir: %s", qPrintable(home));
        return res;
    }

    ProgressCallback progress = callback ? callback : [](const QString &name, const QString &status) {
        qDebug() << "Progress" << name << status;
    };

    if (!units.isEmpty()) {
        for (const QString &unit: units) {
            if (restoreUnit(unit, progress)) {
                res.failedUnits.removeOne(unit);
                res.succededUnits << unit;
            }
        }
    } else {
        //TODO
    }

    return res;
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

bool Vault::backupUnit(const QString &unit, const ProgressCallback &callback)
{
    LibGit::Commit head = LibGit::Branch(&m_vcs, "master").head();
    QString name = unit;
    callback(unit, "begin");
    bool succeded = true; // TODO
    if (succeded) {
        callback(unit, "ok");
        return true;
    }

    callback(unit, "fail");
    m_vcs.clean(LibGit::CleanOptions::Force | LibGit::CleanOptions::RemoveDirectories | LibGit::CleanOptions::IgnoreIgnores, name);
    m_vcs.reset(LibGit::ResetOptions::Hard, head.sha());
    return false;
}

bool Vault::restoreUnit(const QString &unit, const ProgressCallback &callback)
{
    callback(unit, "begin");
    bool succeded = true; // TODO
    if (succeded) {
        callback(unit, "ok");
        return true;
    }

    callback(unit, "fail");
    return false;
}

void Vault::tagSnapshot(const QString &msg)
{
    m_vcs.tag(QLatin1String(">") + msg);

}
