/**
 * @file vault.cpp
 * @brief Vault management API
 * @author Giulio Camuffo <giulio.camuffo@jollamobile.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include "config.hpp"
#include <vault/vault.hpp>

#include <qtaround/util.hpp>
#include <qtaround/os.hpp>
#include <qtaround/error.hpp>
#include <qtaround/debug.hpp>
#include <qtaround/subprocess.hpp>

#include <gittin/commit.hpp>
#include <gittin/branch.hpp>
#include <gittin/repostatus.hpp>
#include <gittin/command.hpp>

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDateTime>
#include <QDir>

#include <tuple>

namespace os = qtaround::os;
namespace subprocess = qtaround::subprocess;
namespace error = qtaround::error;
namespace debug = qtaround::debug;

namespace vault {

static const QMap<File, QString> fileNames = {
    {File::Message, ".message"}
    , {File::VersionTree, ".vault"}
    , {File::VersionRepo, os::path::join(".git", "vault.version")}
    , {File::State, ".vault.state"}
    , {File::Lock, ".vault.lock"}
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

Snapshot::Snapshot(Gittin::Repo *repo, const QString &name)
    : m_tag(repo, QString(">") + name)
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

template <typename ... Args>
std::tuple<int, QString, QString> executeIn
(QString const &dir, QString const &vaultUtilName, Args && ... args)
{
    auto p = subprocess::Process();
    p.setWorkingDirectory(dir);
    p.start(os::path::join(VAULT_LIBEXEC_PATH, vaultUtilName)
            , {std::forward<Args>(args)...});
    p.wait(-1);
    return std::make_tuple(p.rc(), p.stdout(), p.stderr());
}

int Vault::execute(const QVariantMap &options)
{
    debug::debug("Executing", options);
    QString action = options.value("action").toString();
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
        return 0;
    }

    if (!options.contains("vault")) {
        error::raise({{"msg", "Missing option"}, {"name", "vault"}});
    }

    Vault vault(options.value("vault").toString());
    auto l = vault.lock();
    QStringList units{str(options.value("unit")).split(",", QString::SkipEmptyParts)};

    auto unitsResult = [](Result &&res) {
        debug::info("Succeeded units:", res.succededUnits.join(","));
        auto failedCount = res.failedUnits.size();
        if (failedCount)
            debug::warning("Failed units:", res.failedUnits.join(","));

        return failedCount;
    };

    if (action == "init") {
        vault.init(parseKvPairs(options.value("git-config").toString()));
    } else if (action == "export" || action == "backup") {
        return unitsResult(vault.backup
                           (options.value("home").toString(), units
                            , options.value("message").toString()));
    } else if (action == "import" || action == "restore") {
        if (!options.contains("tag")) {
            error::raise({{"msg", "tag should be provided to restore"}});
        }
        return unitsResult(vault.restore
                           (vault.snapshot(options.value("tag").toByteArray())
                            , options.value("home").toString(), units));
    } else if (action == "list-snapshots") {
        auto snapshots = vault.snapshots();
        QTextStream cout{stdout};
        for (const Snapshot &s: snapshots) {
            cout << s.tag().name() << '\n';
        }
    } else if (action == "register") {
        if (!options.contains("data")) {
            error::raise({{"action", action}, {"msg", "Needs data"}});
        }
        auto data = parseKvPairs(str(options["data"]));
        if (options.contains("unit"))
            data["unit"] = options["unit"];
        data["local"] = true;
        debug::info("Register local unit:", data);
        vault.registerConfig(data);
    } else if (action == "unregister") {
        if (!options.contains("unit")) {
            error::raise({{"action", action}, {"msg", "Needs unit name"}});
        }
        vault.unregisterUnit(options.value("unit").toString());
    } else if (action == "gc") {
        auto res = executeIn(vault.root(), "git-vault-gc");
        QTextStream out(stdout, QIODevice::WriteOnly);
        QTextStream err(stderr, QIODevice::WriteOnly);
        out << std::get<1>(res) << endl;
        err << std::get<2>(res) << endl;
        return std::get<0>(res);
    } else {
        error::raise({{"msg", "Unknown action"}, {"action", action}});
    }
    return 0;
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

QString Vault::absolutePath(QString const &relativePath) const
{
    return os::path::join(m_path, relativePath);
}

void Vault::setup(const QVariantMap *config)
{
    debug::debug("Setup vault", config ? *config : QVariantMap{});
    auto l = lock();
    auto createRepo = [this, &l]() {
        debug::debug("Creating repo at", m_path);
        if (!os::path::exists(m_path))
            if (!os::mkdir(m_path))
                error::raise({{"msg", "Can't create repo dir"}, {"path", m_path}});

        l = lock();
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

        if (os::path::exists(m_path)) {
            QDir d(m_path);
            if (!d.entryList(QDir::NoDotAndDotDot).isEmpty())
                error::raise({
                        {"msg", "Vault dir already exists and not empty"}
                        , {"path", m_path}});
        }

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
        auto l = lock();
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
    auto l = lock();
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
    debug::info("Backup units", units, ", home", home);
    auto l = lock();
    Result res;

    if (!os::path::isDir(home)) {
        qWarning("Home is not a dir: %s", qPrintable(home));
        return std::move(res);
    }

    ProgressCallback progress = callback ? callback : [](const QString &name, const QString &status) {
        qDebug() << "Progress" << name << status;
    };

    resetLastSnapshot();
    Gittin::Commit head = Gittin::Branch(&m_vcs, "master").head();

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
            res.succededUnits << unit;
        } else {
            res.failedUnits << unit;
            break;
        }
    }

    if (res.succededUnits.size() == usedUnits.size()) {
        tagSnapshot(message);
    } else {
        debug::warning("Some unit backup is failed, no tag");
        reset(head.sha());
    }
    return res;
}

/// \return snapshot tag name
QString Vault::tagSnapshot(const QString &message)
{
    static const QString timeFormat{"yyyy-MM-ddTHH-mm-ss.zzzZ"};

    auto l = lock();
    // TODO error checking
    m_vcs.checkout("master");

    auto timeTag = QDateTime::currentDateTimeUtc().toString(timeFormat);
    debug::info("Tag", timeTag, message);
    auto metadata = message.isEmpty() ? timeTag : message + '\n' + timeTag;
    writeFile(fileName(File::Message), metadata);
    m_vcs.add(fileName(File::Message));
    Gittin::Commit commit = m_vcs.commit(metadata);
    // message can be theoretically updated later and it should not
    // change commit id, so add it as a not. Initial message is saved
    // in the commit and message file
    commit.addNote(message);

    Snapshot snapshot{&m_vcs, timeTag};
    auto tagName = snapshot.tag().name();
    m_vcs.tag(tagName);
    return tagName;
}

bool Vault::clear(const QVariantMap &options)
{
    auto l = lock();
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
    auto l = lock();
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

/**
 * Reset master branch to the last snapshot commit
 *
 * This action should be done before doing backup to avoid inclusion
 * of any traces/remnants of previous backup tryouts
 */
void Vault::resetLastSnapshot()
{
    auto l = lock();
    resetMaster();

    QString lastSnapshot = "anchor";
    auto snaps = snapshots();
    if (!snaps.isEmpty())
        lastSnapshot = snaps.back().tag().name();

    m_vcs.reset(ResetOptions::Hard, lastSnapshot);
}

Vault::Result Vault::restore(const QString &snapshot, const QString &home, const QStringList &units, const ProgressCallback &callback)
{
    auto l = lock();
    Snapshot ss(&m_vcs, snapshot);
    return restore(ss, home, units, callback);
}

Vault::Result Vault::restore(const Snapshot &snapshot, const QString &home, const QStringList &units, const ProgressCallback &callback)
{
    auto l = lock();
    debug::info("Restore units", units, ", home", home);
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

void Vault::checkout(const QString &snapshotName)
{
    auto name = snapshotName.startsWith('>')
        ? snapshotName
        : '>' + snapshotName;
    Snapshot snapshot(&m_vcs, name);
    m_vcs.checkout(snapshot.tag(), CheckoutOptions::Force);
}

QList<Snapshot> Vault::snapshots() const
{
    auto l = lock();
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
    auto l = lock();
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
    auto l = lock();
    Gittin::Tag tag(&m_vcs, snapshot);
    return tag.notes();
}

QList<QString> Vault::units(QString const & snapshotName) const
{
    auto l = lock();
    QList<QString> result;
    auto res = executeIn(root(), "git-vault-snapshot-units", snapshotName);
    if (!std::get<0>(res)) {
        auto data = str(std::get<1>(res));
        result = data.split("\n", QString::SkipEmptyParts);
    } else {
        QTextStream err(stderr, QIODevice::WriteOnly);
        err << "Error while running git-vault-snapshot-units\n";
        err << std::get<1>(res);
        err << std::get<2>(res);
    }
    return result;
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

// TODO move to the library
Gittin::Commit commitEvenIfClean(Gittin::Repo &vcs, QString const &msg)
{
    auto options = QStringList() << "--allow-empty" << "-m" << msg;
    vcs.command("commit", options);
    auto log_opts = QStringList() << "-n1" << "--oneline" << "--no-abbrev-commit";
    auto commit = vcs.command("log", log_opts)->stdout();
    // TODO: FIX: object takes unsafe pointer to another object
    return Gittin::Commit(&vcs, commit.split(' ').first());
}

struct Unit
{
    Unit(const QString &unit, const QString &home
         , Gittin::Repo *vcs, const config::Unit &config);

    void execScript(const QString &action);
    void linkBlob(const QString &file);
    void backup();
    void restore();

    QString m_home;
    QString m_unit;
    QDir m_root;
    Gittin::Repo *m_vcs;
    QString m_blobs;
    QString m_data;
    config::Unit m_config;
};

/// \return (unit, isOk, reason)
std::tuple<QString, bool, QString> Vault::backupUnit
(const QString &home, const QString &unit)
{
    auto l = lock();
    QString reason;
    auto onDone = [&reason](const QString &, const QString &result) {
        reason = result;
    };
    // TODO error check
    m_vcs.checkout("master");
    auto isOk = backupUnit(home, unit, onDone);
    return std::make_tuple(unit, isOk, reason);
}

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
    } catch (error::Error const &err) {
        debug::error("Unit backup error:", err.m);
        callback(unit, str(err.m.value("reason", "fail")));
        m_vcs.clean(CleanOptions::Force | CleanOptions::RemoveDirectories | CleanOptions::IgnoreIgnores, unit);
        m_vcs.reset(ResetOptions::Hard, head.sha());
        return false;
    }

    return true;
}

/// \return (unit, isOk, reason)
std::tuple<QString, bool, QString> Vault::restoreUnit
(const QString &snapshot, const QString &home, const QString &unit)
{
    auto l = lock();
    QString reason;
    auto onDone = [&reason](const QString &, const QString &result) {
        reason = result;
    };
    // TODO error check
    checkout(snapshot);
    auto isOk = restoreUnit(home, unit, onDone);
    return std::make_tuple(unit, isOk, reason);
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

std::tuple<int, QString, QString>
Vault::exportSnapshot(const QString &snapshot, const QString &dstDir)
{
    debug::debug("Export snapshot", snapshot, "to", dstDir);
    auto l = lock();
    auto snapshotName = snapshot;
    if (snapshotName[0] != '>')
        snapshotName = QString(">") + snapshot;

    return executeIn(root(), "git-vault-export", snapshotName, dstDir);
}

/**
 * \note thread-unsafe
 */
Lock Vault::lock() const
{
    if (!exists())
        return Lock{};
    auto handle = m_barrier.lock();
    if (!handle) {
        int timeout = 1 * 1000;
        auto lock_fname = absolutePath(fileName(File::Lock));
        auto gotLock = [&handle](os::FileLock l) { handle = box(std::move(l)); };
        auto locker = os::tryLock(lock_fname, gotLock, timeout);
        while (locker) {
            // max 8s timeout
            if (timeout <= 4 * 1000)
                timeout *= 2;
            locker = os::tryLock(std::move(locker), gotLock, timeout);
        }
        m_barrier = handle;
    }
    return handle;
}


Unit::Unit(const QString &unit, const QString &home
           , Gittin::Repo *vcs, const config::Unit &config)
    : m_home(home)
    , m_unit(unit)
    , m_root(QDir(os::path::join(vcs->path(), unit)))
    , m_vcs(vcs)
    , m_config(config)
{
    m_blobs = os::path::join(m_root.absolutePath(), "blobs");
    m_data = os::path::join(m_root.absolutePath(), "data");
}

void Unit::execScript(const QString &action)
{
    QString script = m_config.script();
    debug::info("SCRIPT>>>", script, "action", action);
    if (!QFileInfo(script).isExecutable()) {
        error::raise({{"msg", "Should be executable"}, {"script", script}});
    }
    QStringList args = { "--action", action,
                         "--name", m_unit,
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

static const int sha1HashLength = 40;

void Unit::linkBlob(const QString &file)
{
    QString blobStorage = os::path::join(m_vcs->path(), ".git", "blobs");
    QByteArray sha = m_vcs->hashObject(file);
    if (sha.length() != sha1HashLength)
        error::raise({{"msg", "Can't hash object"},
                    {"file", file}, {"hash", str(sha)}});

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

void Unit::backup()
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
        debug::info("No changes for the unit ", name);
        // but still commit to track user's intention to backup unit
    } else {
        // add all only in data dir to avoid blobs to get into git
        // objects storage
        m_vcs->add(os::path::join(m_root.path(), "data"), Gittin::AddOptions::All);
        status = m_vcs->status(m_root.path());
        if (status.hasDirtyFiles()) {
            error::raise({{"msg", "Dirty tree"}, {"dir", m_root.path()}/*, {"status", status_dump(status)}*/});
        }
    }
    commitEvenIfClean(*m_vcs, Snapshot(m_vcs, name).tag().name());
    status = m_vcs->status(m_root.path());
    if (!status.isClean()) {
        error::raise({{"msg", "Not fully commited"}, {"dir", m_root.path()}/*, {"status", status_dump(status)}*/});
    }
}

void Unit::restore()
{
    if (!m_root.exists()) {
        error::raise({{"reason", "absent"}, {"name", m_unit}});
    }
    execScript("import");
}


}
