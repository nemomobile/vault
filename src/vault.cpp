
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDateTime>
#include <QDir>

#include <libgit/commit.h>
#include <libgit/branch.h>
#include <libgit/repostatus.h>

#include "vault.hpp"
#include "os.hpp"
#include "error.hpp"
#include "debug.hpp"
#include "subprocess.hpp"

static const struct {
    int tree;
    int repository;
} version = { 2, 2 };


Snapshot::Snapshot(const LibGit::Tag &tag)
        : m_tag(tag)
{
}



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
            if (backupUnit(home, unit, progress)) {
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

Vault::Result Vault::restore(const Snapshot &snapshot, const QString &home, const QStringList &units, const ProgressCallback &callback)
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

    m_vcs.checkout(snapshot.tag());
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

QList<Snapshot> Vault::snapshots() const
{
    QList<Snapshot> list;
    for (const LibGit::Tag &tag: m_vcs.tags()) {
        if (!tag.name().isEmpty() && tag.name().startsWith('>')) {
            list << Snapshot(tag);
        }
    }
    return list;
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

struct Unit
{
    Unit(const QString &unit, LibGit::Repo *vcs)
        : m_unit(unit)
        , m_root(QDir(vcs->path() + "/" + unit))
        , m_vcs(vcs)
    {
        m_blobs = m_root.absolutePath() + "/blobs";
        m_data = m_root.absolutePath() + "/data";
    }

    void execScript(const QString &action)
    {
        QString script = m_unit; //FIXME
        debug::info("SCRIPT>>>", script, "action", action);
        if (!QFileInfo(script).isExecutable()) {
            error::raise({{"msg", "Should be executable"}, {"script", script}});
        }
        QStringList args = { "--action", action,
                             "--dir", QDir(m_data).absolutePath(),
                             "--bin-dir", QDir(m_blobs).absolutePath(),
                             "--home-dir", m_home };

        subprocess::Process ps;
        ps.start(script, args);
        ps.wait(-1);

        debug::Level level = ps.rc() ? debug::Level::Error : debug::Level::Info;

        debug::print_ge(level, "RC", ps.rc());
        debug::print_ge(level, "STDOUT", ps.stdout());
        debug::print_ge(level, "<<STDOUT");
        debug::print_ge(level, "STDERR", ps.stderr());
        debug::print_ge(level, "<<STDERR");
        debug::print_ge(level, "<<<SCRIPT", script, "action", action, "is done");
        if (ps.rc()) {
            QString msg = "Backup script " + script + " exited with rc=" + ps.rc();
            error::raise({{"msg", msg}, {"stdout", ps.stdout()}, {"stderr", ps.stderr()}});
        }
    }

    void linkBlob(const QString &file)
    {
        QString blobStorage = m_vcs->path() + "/.git/blobs/";
        QByteArray sha = m_vcs->hashObject(file);
        QString blobDir = blobStorage + sha.left(2);
        QString blobFName = blobDir + "/" + sha.mid(2);
        QString linkFName = m_vcs->path() + "/" + file;

        if (!os::path::exists(blobDir)) {
            os::mkdir(blobDir);
        }

        QDateTime origTime = os::lastModified(linkFName);
        if (os::path::isFile(blobFName)) {
            os::unlink(linkFName);
        } else {
            os::rename(linkFName, blobFName);
        }
        os::setLastModified(blobFName, origTime);
        QString target = os::path::relative(blobFName, os::path::dirName(linkFName));
        os::symlink(target, linkFName);
        if (!os::path::isSymLink(linkFName) && os::path::isFile(blobFName)) {
            error::raise({{"msg", "Blob should be symlinked"},
                          {"link", linkFName}, {"target", target}});
        }
        m_vcs->add(file);
    }

    void backup(const QString &home)
    {
        m_home = home;
        QString name = m_unit; //FIXME

        // cleanup directories for data and blobs in
        // the repository
        os::rmtree(m_blobs);
        os::rmtree(m_data);
        os::mkdir(m_root.absolutePath());
        os::mkdir(m_blobs);
        os::mkdir(m_data);

        execScript("export");

        qDebug()<<"bac"<<m_unit<<m_blobs;
        LibGit::RepoStatus status = m_vcs->status(m_root.path() + "/blobs");
        for (const LibGit::RepoStatus::File &file: status.files()) {
            if (file.index == ' ' && file.workTree == 'D') {
                m_vcs->rm(file.file);
                break;
            }

            QString fname = QFileInfo(file.file).fileName();
            QString prefix = m_unit; //FIXME
            if (fname.length() >= prefix.length() && fname.startsWith(prefix)) {
                m_vcs->add(file.file);
                break;
            }

            linkBlob(file.file);
        }

        if (m_vcs->status(m_root.path()).isClean()) {
            debug::info("Nothing to backup for ", name);
            return;
        }

        // add all only in data dir to avoid blobs to get into git
        // objects storage
        m_vcs->add(m_root.path() + "/data", LibGit::AddOptions::All);
        status = m_vcs->status(m_root.path());
        if (status.hasDirtyFiles()) {
            error::raise({{"msg", "Dirty tree"}, {"dir", m_root.path()}/*, {"status", status_dump(status)}*/});
        }

        m_vcs->commit(">" + name);
        status = m_vcs->status(m_root.path());
        if (!status.isClean()) {
            error::raise({{"msg", "Not fully commited"}, {"dir", m_root.path()}/*, {"status", status_dump(status)}*/});
        }
    }

    bool restore()
    {
        if (!m_root.exists()) {
            debug::error("no such directory", m_root.path());
            return false;
        }
        execScript("import");
        return true;
    }

    QString m_home;
    QString m_unit;
    QDir m_root;
    LibGit::Repo *m_vcs;
    QString m_blobs;
    QString m_data;
};

bool Vault::backupUnit(const QString &home, const QString &unit, const ProgressCallback &callback)
{
    LibGit::Commit head = LibGit::Branch(&m_vcs, "master").head();
    QString name = unit;

    try {
        callback(unit, "begin");
        Unit u(unit, &m_vcs);
        u.backup(home);
        callback(unit, "ok");
    } catch (error::Error err) {
        debug::error(err.what(), "\n");
        callback(unit, err.m.contains("reason") ? err.m.value("reason").toString() : "fail");
        m_vcs.clean(LibGit::CleanOptions::Force | LibGit::CleanOptions::RemoveDirectories | LibGit::CleanOptions::IgnoreIgnores, name);
        m_vcs.reset(LibGit::ResetOptions::Hard, head.sha());
        return false;
    }

    return true;
}

bool Vault::restoreUnit(const QString &unit, const ProgressCallback &callback)
{
    callback(unit, "begin");
    Unit u(unit, &m_vcs);
    if (u.restore()) {
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
