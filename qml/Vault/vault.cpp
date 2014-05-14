#include <os.hpp>
#include <debug.hpp>
#include <vault/vault.hpp>
#include <vault/config.hpp>
#include <transfer.hpp>

#include "vault.hpp"

class Worker : public QObject
{
    Q_OBJECT
public:
    Worker()
        : QObject()
        , m_vault(nullptr)
        , m_transfer(nullptr)
    {
    }

    ~Worker()
    {
        delete m_vault;
        delete m_transfer;
    }

    Q_INVOKABLE void init(const QString &root)
    {
        m_vault = new vault::Vault(root);
        QVariantMap options = {{ "user.name", "Some Sailor"}, {"user.email", "sailor@jolla.com"}};
        if (!m_vault->exists()) {
            m_vault->init(options);
        } else {
            if (m_vault->isInvalid()) {
                error::raise({{"msg", "vault is invalid"}});
            }
        }
        syncConfig();
    }

    void syncConfig()
    {
        vault::config::Config *global = vault::config::Config::global();
        m_vault->config().update(global->units());
    }

    Q_INVOKABLE void restore(const QString &home, const QStringList &units, const QString &tag)
    {
        debug::debug("Restore: home", home);
        m_vault->restore(tag, home, units, [this](const QString unit, const QString &status) {
            emit progress(Vault::Restore, {{"unit", unit}, {"status", status}});
        });
        emit done(Vault::Restore);
    }

    Q_INVOKABLE void backup(const QString &home, const QStringList &units, const QString &message)
    {
        debug::debug("Backup: home", home);
        m_vault->backup(home, units, message, [this](const QString unit, const QString &status) {
            emit progress(Vault::Backup, {{"unit", unit}, {"status", status}});
        });
        emit done(Vault::Backup);
    }

    QStringList snapshots() const
    {
        QStringList snaps;
        for (const vault::Snapshot &ss: m_vault->snapshots()) {
            snaps << ss.tag().name();
        }
        return snaps;
    }

    Q_INVOKABLE bool destroy()
    {
        debug::debug("Destroy vault");
        QVariantMap msg = {{"destroy", true}, {"ignore_snapshots", true}};
        return m_vault->clear(msg);
    }

    Q_INVOKABLE void eiPrepare(Vault::ImportExportAction action, const QString &path)
    {
        if (!m_transfer) {
            m_transfer = new CardTransfer;
        }
        try {
            CardTransfer::Action ac = action == Vault::Import ? CardTransfer::Import : CardTransfer::Export;
            m_transfer->init(ac, path);
            emit done(Vault::ExportImportPrepare);
        } catch (error::Error e) {
            emit error(Vault::ExportImportPrepare, e.what());
        }
    }

    Q_INVOKABLE void eiExecute()
    {
        if (!m_transfer) {
            emit error(Vault::ExportImportExecute, "exportImportPrepare was not called");
            return;
        }
        try {
            m_transfer->execute([this](QVariantMap &&map) {
                emit progress(Vault::ExportImportExecute, map);
            });
            emit done(Vault::ExportImportExecute);
        } catch (error::Error e) {
            emit error(Vault::ExportImportExecute, e.what());
        }
    }

signals:
    void progress(Vault::Operation op, const QVariantMap &map);
    void error(Vault::Operation op, const QString &error);
    void done(Vault::Operation op);

public:
    vault::Vault *m_vault;
    CardTransfer *m_transfer;
};


Vault::Vault(QObject *p)
     : QObject(p)
     , m_worker(nullptr)
     , m_home(os::home())
     , m_root(os::path::join(m_home, ".vault"))
{
}

Vault::~Vault()
{
    m_workerThread.quit();
    m_workerThread.wait();
    delete m_worker;
}

QString Vault::root() const
{
    return m_root;
}

QString Vault::backupHome() const
{
    return m_home;
}

void Vault::setRoot(const QString &root)
{
    if (m_root != root) {
        m_root = root;
        emit rootChanged();
    }
}

void Vault::setBackupHome(const QString &home)
{
    if (m_home != home) {
        m_home = home;
        emit backupHomeChanged();
    }
}

void Vault::initWorker(bool reload)
{
    debug::info((reload ? "re" : ""), "connect to vault");
    debug::debug("Vault in", m_root, ", Home is", m_home);
    debug::info("Init vault actor, exist", m_worker, "reload=", reload);
    if (!m_worker) {
        m_worker = new Worker;
        m_worker->moveToThread(&m_workerThread);
        m_workerThread.start();
        reload = false;
        connect(m_worker, &Worker::progress, this, &Vault::progress);
        connect(m_worker, &Worker::error, this, &Vault::error);
        connect(m_worker, &Worker::done, this, &Vault::done);
    }
    try {
        m_worker->init(m_root);
        emit done(Vault::Connect);
    } catch (error::Error e) {
        emit error(Vault::Connect, e.what());
    }
}

void Vault::connectVault(bool reconnect)
{
    if (m_worker && !reconnect) {
        emit done(Vault::Connect);
        return;
    }

    initWorker(reconnect);
}

void Vault::startRestore(const QString &tag, const QStringList &units)
{
    if (units.isEmpty()) {
        debug::info("Nothing to restore");
        emit done(Vault::Restore);
        return;
    }

    QMetaObject::invokeMethod(m_worker, "restore", Q_ARG(QString, m_home), Q_ARG(QStringList, units), Q_ARG(QString, tag));
}

void Vault::startBackup(const QString &message, const QStringList &units)
{
    if (units.isEmpty()) {
        debug::info("Nothing to backup");
        emit done(Vault::Backup);
        return;
    }

    QMetaObject::invokeMethod(m_worker, "backup", Q_ARG(QString, m_home), Q_ARG(QStringList, units), Q_ARG(QString, message));
}

QStringList Vault::snapshots() const
{
    return m_worker->snapshots();
}

QStringList Vault::units() const
{
    QStringList units;
    for (auto &u: m_worker->m_vault->config().units()) {
        units << u.name();
    }
    return units;
}

void Vault::resetHead()
{
    m_worker->m_vault->reset();
}

void Vault::removeSnapshot(const QString &name)
{
    debug::debug("Requesting snapshot removal:", name);
    for (vault::Snapshot ss: m_worker->m_vault->snapshots()) {
        if (ss.tag().name() == name)  {
            ss.remove();
            break;
        }
    }

    if (snapshots().size() == 0) {
        // TODO This action should be done only until proper
        // garbage collection will be introduced
        debug::warning("Last snapshot is removed, destroy storage to conserve space");
        m_worker->destroy();
        try {
            initWorker(true);
        } catch (error::Error e) {
            debug::error("Error reconnecting", e.what());
            emit error(Vault::RemoveSnapshot, e.what());
        }
    } else {
        debug::debug("There are some snapshots, continue");
    }
}

void Vault::exportImportPrepare(ImportExportAction action, const QString &path)
{
    QMetaObject::invokeMethod(m_worker, "eiPrepare", Q_ARG(int, action), Q_ARG(QString, path));
}

void Vault::exportImportExecute()
{
    QMetaObject::invokeMethod(m_worker, "eiExecute");
}

#include "vault.moc"
