/**
 * @file vault.cpp
 * @brief Vault management API
 * @author Giulio Camuffo <giulio.camuffo@jollamobile.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <vault/vault.hpp>

#include <qtaround/util.hpp>
#include <qtaround/os.hpp>
#include <qtaround/error.hpp>
#include <qtaround/debug.hpp>
#include <qtaround/subprocess.hpp>

#include <gittin/commit.hpp>
#include <gittin/branch.hpp>
#include <gittin/repostatus.hpp>

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDateTime>
#include <QDir>

namespace vault {

static const QMap<File, QString> fileNames = {
    {File::Message, ".message"}
    , {File::VersionTree, ".vault"}
    , {File::VersionRepo, os::path::join(".git", "vault.version")}
    , {File::State, ".vault.state"}
};

QString fileName(File id)
{
    return fileNames[id];
};

using Gittin::CleanOptions;
using Gittin::ResetOptions;
using Gittin::CheckoutOptions;

namespace version {
    static const int tree = 3;
    static const int repository = 2;
}

Snapshot::Snapshot(const Gittin::Tag &tag)
        : m_tag(tag)
{
}

QString Snapshot::name() const
{
    return m_tag.name().mid(1);
}

void Snapshot::remove()
{
    m_tag.destroy();
}



Vault::Vault(const QString &path)
     : m_path(path)
     , m_blobStorage(os::path::join(path, ".git", "blobs"))
     , m_vcs(path)
     , m_config(&m_vcs)
{
    setup(nullptr);
}

static QVariantMap parseKvPairs(const QString &cfg)
{
    QVariantMap data;
    for (const QString v: cfg.split(',')) {
        QStringList kv = v.split('=');
        if (kv.size() == 2 && !kv.at(0).isEmpty()) {
            data[kv.at(0)] = kv.at(1);
        }
    }
    return data;
}

config::Vault Vault::config()
{
    return config::Vault(&m_vcs);
}

Vault::UnitPath Vault::unitPath(const QString &name) const
{
    UnitPath res;
    res.path = os::path::join(m_vcs.path(), name);
    res.bin = os::path::join(res.path, "blobs");
    res.data = os::path::join(res.path, "data");
    return res;
};

bool Vault::UnitPath::exists() const
{
    return os::path::isDir(path);
}

void Vault::execute(const QVariantMap &options)
{
    QString action = options.value("action").toString();
    debug::debug("Action:", action);
    if (options.value("global").toBool()) {
        debug::debug("Global action");
        if (action == "register") {
            if (!options.contains("data")) {
                error::raise({{"action", action}, {"msg", "Needs data" }});
            }

            QString cfg = options.value("data").toString();
            QVariantMap data = parseKvPairs(cfg);
            if (options.contains("unit")) {
                data["name"] = options.value("unit");
            }
            config::global()->set(data);
        } else if (action == "unregister") {
            if (!options.contains("unit")) {
                error::raise({{"action", action}, {"msg", "Needs unit name"}});
            }

            config::global()->rm(options.value("unit").toString());
        } else {
            error::raise({{"msg", "Unknown action"}, {"action", action}});
        }
        return;
    }

    if (!options.contains("vault")) {
        error::raise({{"msg", "Missing option"}, {"name", "vault"}});
    }

    Vault vault(options.value("vault").toString());
    QStringList units{str(options.value("unit")).split(",", QString::SkipEmptyParts)};

    if (action == "init") {
        vault.init(parseKvPairs(options.value("git_config").toString()));
    } else if (action == "export" || action == "backup") {
        vault.backup(options.value("home").toString(), units, options.value("message").toString());
    } else if (action == "import" || action == "restore") {
        if (!options.contains("tag")) {
            error::raise({{"msg", "tag should be provided to restore"}});
        }
        vault.restore(vault.snapshot(options.value("tag").toByteArray()), options.value("home").toString(), units);
    } else if (action == "list-snapshots") {
        auto snapshots = vault.snapshots();
        for (const Snapshot &s: snapshots) {
            qDebug() << s.tag().name();
        }
    } else if (action == "register") {
        if (!options.contains("data")) {
            error::raise({{"action", action}, {"msg", "Needs data"}});
        }
        qDebug()<<parseKvPairs(options.value("data").toString());
        vault.registerConfig(parseKvPairs(options.value("data").toString()));
    } else if (action == "unregister") {
        if (!options.contains("unit")) {
            error::raise({{"action", action}, {"msg", "Needs unit name"}});
        }
        vault.unregisterUnit(options.value("unit").toString());
    } else {
        error::raise({{"msg", "Unknown action"}, {"action", action}});
    }
}

void Vault::registerConfig(const QVariantMap &config)
{
    m_vcs.checkout("master", Gittin::CheckoutOptions::Force);
    reset("master");
    m_config.set(config);
}

void Vault::unregisterUnit(const QString &unit)
{
    m_vcs.checkout("master", Gittin::CheckoutOptions::Force);
    reset("master");
    m_config.rm(unit);
}

void Vault::setVersion(File dst, int ver)
{
    auto fname = fileName(dst);
    if (!writeFile(fname, str(ver)))
        error::raise({{"msg", "Can't save version"}, {"path", fname}});
}

QString Vault::readFile(const QString &relPath)
{
    return QString::fromUtf8(os::read_file(os::path::join(m_path, relPath)));
}

int Vault::getVersion(File src)
{
    return readFile(fileName(src)).toInt();
}

QString Vault::absolutePath(QString const &relativePath)
{
    return os::path::join(m_path, relativePath);
}

void Vault::setup(const QVariantMap *config)
{
    auto createRepo = [this]() {
        debug::debug("Creating repo at", m_path);
        if (!os::path::exists(m_path))
            if (!os::mkdir(m_path))
                error::raise({{"msg", "Can't create repo dir"}, {"path", m_path}});

        if (!m_vcs.init())
            error::raise({{"msg", "Can't init git repo"}, {"path", m_path}});
    };

    auto setupGitConfig = [this, config]() {
        debug::debug("Setup git config", *config);
        m_vcs.setConfigValue("status.showUntrackedFiles", "all");
        for (auto it = config->begin(); it != config->end(); ++it)
            m_vcs.setConfigValue(it.key(), it.value().toString());
    };

    auto excludeServiceFiles = [this]() {
        auto path = ".git/info/exclude";
        if (!writeFile(path, ".vault.*\n.units/"))
            error::raise({{"msg", "Can't write exclude info"}, {"path", path}});
    };

    auto initVersions = [this]() {
        debug::debug("Init vault versions");
        setVersion(File::VersionTree, version::tree);
        m_vcs.add(fileName(File::VersionTree));
        m_vcs.commit("anchor");
        m_vcs.tag("anchor");

        if (!os::path::exists(m_blobStorage))
            if (!os::mkdir(m_blobStorage))
                error::raise({{"msg", "Can't create blob storage"},
                            {"path", m_blobStorage}});

        setVersion(File::VersionRepo, version::repository);
    };

    auto updateTreeVersion = [this](unsigned current) {
        debug::info("Updating tree version from", current, "to", version::tree);

        if (current < 2) {
            debug::info("since v2 there is no 'latest' tag");
            snapshot("latest").remove();
        }

        if (current < 3) {
            debug::info("Since v3 information about units is not "
                        "under version control and moved to the .units dir."
                        "Old .modules is deprecated.");
        }

        setVersion(File::VersionTree, version::tree);
        m_vcs.add(fileName(File::VersionTree));
        m_vcs.commit("vault format version");
    };

    auto updateRepoVersion = [this, &excludeServiceFiles](unsigned current) {
        debug::info("Updating repo version from", current, "to", version::repository);
        if (current < 1) {
            // state tracking file is appeared in version 1
            // all .vault.* are also going to be ignored
            excludeServiceFiles();
        }
        setVersion(File::VersionRepo, version::repository);
    };

    auto syncConfigGlobal = [this]() {
        auto global = vault::config::global();
        if (global)
            this->config().update(global->units());
    };

    if (exists() && !isInvalid()) {
        debug::debug("Repository exists and it is not invalid, setup");

        auto v = getVersion(File::VersionTree);
        if (v < version::tree)
            updateTreeVersion(v);

        v = getVersion(File::VersionRepo);
        if (v < version::repository)
            updateRepoVersion(v);

        excludeServiceFiles();
        setState("new");
        if (config)
            setupGitConfig();

        syncConfigGlobal();
    } else if (config) {
        debug::info("Repository initialization is requested");

        if (os::path::exists(m_path))
            error::raise({{"msg", "Vault dir already exists, can't create"}, {"path", m_path}});

        try {
            createRepo();
            setupGitConfig();
            excludeServiceFiles();
            initVersions();
            syncConfigGlobal();
            setState("new");
        } catch (...) {
            os::rmtree(m_path);
            throw;
        }
    }
}

bool Vault::init(const QVariantMap &config)
{
    try {
        setup(&config);
        return true;
    } catch (std::exception const &e) {
        debug::error("Error:",  e.what(), ", initializing repository", m_path);
    } catch (...) {
        debug::error("Unknown error initializing repository", m_path);
    }
    return false;
}

bool Vault::ensureValid()
{
    if (!os::path::exists(absolutePath(".git"))) {
        debug::info("Can't find .git", m_path);
        return false;
    }
    if (!os::path::exists(m_blobStorage)) {
        debug::info("Can't find blobs storage", m_blobStorage);
        return false;
    }

    auto versionTreeFile = absolutePath(fileName(File::VersionTree));
    if (!os::path::isFile(versionTreeFile)) {
        resetMaster();
        if (!os::path::isFile(versionTreeFile)) {
            debug::info("Can't find .vault anchor in ", m_path);
            return false;
        }
    }
    return true;
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

    resetMaster();

    QStringList usedUnits = units;
    if (units.isEmpty()) {
        debug::info("Units list is not supplied, backup all units");
        QMap<QString, config::Unit> units = config().units();
        for (auto i = units.begin(); i != units.end(); ++i) {
            usedUnits << i.key();
        }
    }
    for (const QString &unit: usedUnits) {
        if (backupUnit(home, unit, progress)) {
            res.failedUnits.removeOne(unit);
            res.succededUnits << unit;
        }
    }

    QString timeTag = QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH-mm-ss.zzzZ");
    qDebug()<<timeTag<<message;
    QString msg = message.isEmpty() ? timeTag : message + '\n' + timeTag;
    writeFile(fileName(File::Message), msg);
    m_vcs.add(fileName(File::Message));
    Gittin::Commit commit = m_vcs.commit(timeTag + '\n' + msg);
    commit.addNote(message);
    tagSnapshot(timeTag);

    return res;
}

bool Vault::clear(const QVariantMap &options)
{
    if (!os::path::isDir(m_path)) {
        debug::info("vault.clear:", "Path", m_path, "is not a dir");
        return false;
    }

    auto destroy = [this]() {
        return !os::rmtree(m_path) && !os::path::exists(m_path);
    };

    if (isInvalid()) {
        if (!options.value("clear_invalid").toBool()) {
            debug::info("vault.clear:", "Can't clean invalid vault implicitely");
            return false;
        }
        if (options.value("destroy").toBool()) {
            debug::info("vault.clear:", "Destroying invalid vault at", m_path);
            return destroy();
        }
    }
    if (options.value("destroy").toBool()) {
        if (!options.value("ignore_snapshots").toBool() && snapshots().size()) {
            debug::info("vault.clear:", "Can't ignore snapshots", m_path);
            return false;
        }
        debug::info("vault.clear:", "Destroying vault storage at", m_path);
        return destroy();
    }
    return false;
}

void Vault::reset(const QByteArray &treeish)
{
    m_vcs.clean(CleanOptions::Force | CleanOptions::RemoveDirectories);
    if (!treeish.isEmpty()) {
        m_vcs.reset(ResetOptions::Hard, treeish);
    } else {
        m_vcs.reset(ResetOptions::Hard);
    }
}

void Vault::resetMaster()
{
    reset();
    m_vcs.checkout("master", CheckoutOptions::Force);
}

Vault::Result Vault::restore(const QString &snapshot, const QString &home, const QStringList &units, const ProgressCallback &callback)
{
    Snapshot ss(Gittin::Tag(&m_vcs, QString(">") + snapshot));
    return restore(ss, home, units, callback);
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
    QStringList usedUnits = units;
    if (units.isEmpty()) {
        QMap<QString, config::Unit> units = config().units();
        for (auto i = units.begin(); i != units.end(); ++i) {
            usedUnits << i.key();
        }
    }

    debug::debug("Restore units:", usedUnits);
    for (const QString &unit: usedUnits) {
        if (restoreUnit(home, unit, progress)) {
            res.failedUnits.removeOne(unit);
            res.succededUnits << unit;
        }
    }

    m_vcs.checkout("master");
    return res;
}

QList<Snapshot> Vault::snapshots() const
{
    auto tags = m_vcs.tags();
    QList<Snapshot> list;
    for (const Gittin::Tag &tag: tags) {
        if (!tag.name().isEmpty() && tag.name().startsWith('>')) {
            list << Snapshot(tag);
        }
    }
    return list;
}

Snapshot Vault::snapshot(const QByteArray &tagName) const
{
    auto tags = m_vcs.tags();
    for (const Gittin::Tag &tag: tags) {
        if (tag.name() == tagName) {
            return Snapshot(tag);
        }
    }
    error::raise({{"msg", "Wrong snapshot tag"}, {"tag", tagName}});
    return Snapshot(m_vcs.tags().first());
}

QString Vault::notes(const QString &snapshot)
{
    Gittin::Tag tag(&m_vcs, snapshot);
    return tag.notes();
}

bool Vault::exists() const
{
    return os::path::isDir(m_path);
}

bool Vault::isInvalid()
{
    QString storage(absolutePath(".git"));
    QString blob_storage(os::path::join(storage, "blobs"));
    if (!os::path::exists(storage) || !os::path::exists(blob_storage)) {
        return true;
    }

    QString anchor(absolutePath(fileName(File::VersionTree)));
    if (!os::path::isFile(anchor)) {
        resetMaster();
        if (!os::path::isFile(anchor))
            return true;
    }
    return false;
};

bool Vault::writeFile(const QString &path, const QString &content)
{
    QFile file(absolutePath(path));
    if (!content.endsWith('\n')) {
        return os::write_file(absolutePath(path), content + '\n');
    }
    return os::write_file(absolutePath(path), content);
}

bool Vault::setState(const QString &state)
{
    return writeFile(fileName(File::State), state);
}

struct Unit
{
    Unit(const QString &unit, const QString &home, Gittin::Repo *vcs, const config::Unit &config)
        : m_home(home)
        , m_unit(unit)
        , m_root(QDir(os::path::join(vcs->path(), unit)))
        , m_vcs(vcs)
        , m_config(config)
    {
        m_blobs = os::path::join(m_root.absolutePath(), "blobs");
        m_data = os::path::join(m_root.absolutePath(), "data");
    }

    void execScript(const QString &action)
    {
        QString script = m_config.script();
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
        QString blobStorage = os::path::join(m_vcs->path(), ".git", "blobs");
        QByteArray sha = m_vcs->hashObject(file);
        QString blobDir = os::path::join(blobStorage, sha.left(2));
        QString blobFName = os::path::join(blobDir, sha.mid(2));
        QString linkFName = os::path::join(m_vcs->path(), file);

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

    void backup()
    {
        QString name = m_config.name();

        // cleanup directories for data and blobs in
        // the repository
        os::rmtree(m_blobs);
        os::rmtree(m_data);
        os::mkdir(m_root.absolutePath());
        os::mkdir(m_blobs);
        os::mkdir(m_data);

        execScript("export");

        Gittin::RepoStatus status = m_vcs->status(os::path::join(m_root.path(), "blobs"));
        for (const Gittin::RepoStatus::File &file: status.files()) {
            if (file.index == ' ' && file.workTree == 'D') {
                m_vcs->rm(file.file);
                continue;
            }

            QString fname = QFileInfo(file.file).fileName();
            QString prefix = config::prefix;
            if (fname.length() >= prefix.length() && fname.startsWith(prefix)) {
                m_vcs->add(file.file);
                continue;
            }

            linkBlob(file.file);
        }

        if (m_vcs->status(m_root.path()).isClean()) {
            debug::info("Nothing to backup for ", name);
            return;
        }

        // add all only in data dir to avoid blobs to get into git
        // objects storage
        m_vcs->add(os::path::join(m_root.path(), "data"), Gittin::AddOptions::All);
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

    void restore()
    {
        if (!m_root.exists()) {
            error::raise({{"reason", "absent"}, {"name", m_unit}});
        }
        execScript("import");
    }

    QString m_home;
    QString m_unit;
    QDir m_root;
    Gittin::Repo *m_vcs;
    QString m_blobs;
    QString m_data;
    config::Unit m_config;
};

bool Vault::backupUnit(const QString &home, const QString &unit, const ProgressCallback &callback)
{
    Gittin::Commit head = Gittin::Branch(&m_vcs, "master").head();

    try {
        debug::info("Backup unit", unit);
        if (unit.isEmpty())
            error::raise({{"msg", "Trying to backup unit w/o name"}});

        callback(unit, "begin");
        Unit u(unit, home, &m_vcs, config().units().value(unit));
        u.backup();
        callback(unit, "ok");
    } catch (error::Error err) {
        debug::error(err.what(), "\n");
        callback(unit, err.m.contains("reason") ? err.m.value("reason").toString() : "fail");
        m_vcs.clean(CleanOptions::Force | CleanOptions::RemoveDirectories | CleanOptions::IgnoreIgnores, unit);
        m_vcs.reset(ResetOptions::Hard, head.sha());
        return false;
    }

    return true;
}

bool Vault::restoreUnit(const QString &home, const QString &unit, const ProgressCallback &callback)
{
    try {
        debug::info("Restore unit", unit);
        if (unit.isEmpty())
            error::raise({{"msg", "Trying to restore unit w/o name"}});

        callback(unit, "begin");
        Unit u(unit, home, &m_vcs, config().units().value(unit));
        u.restore();
        callback(unit, "ok");
    } catch (error::Error err) {
        debug::error(err.what(), "\n");
        callback(unit, err.m.contains("reason") ? err.m.value("reason").toString() : "fail");
        return false;
    }

    return true;
}

void Vault::tagSnapshot(const QString &msg)
{
    m_vcs.tag(QLatin1String(">") + msg);
}

}
