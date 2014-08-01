#include <qtaround/os.hpp>
#include <qtaround/debug.hpp>
#include <vault/vault.hpp>
#include <vault/config.hpp>
#include <transfer.hpp>

#include <QJSValue>

#include "vault.hpp"

Q_DECLARE_METATYPE(Vault::Operation)
static const int _vault_operation_ __attribute__((unused))
= qRegisterMetaType<Vault::Operation>();
Q_DECLARE_METATYPE(Vault::ImportExportAction)
static const int _vault_importexportaction_ __attribute__((unused))
= qRegisterMetaType<Vault::ImportExportAction>();

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
        if (!m_vault->init(options)) {
            error::raise({{"msg", "Can't init vault"}, {"path", root}});
        }
        syncConfig();
    }

    void syncConfig()
    {
        vault::config::Config *global = vault::config::global();
        m_vault->config().update(global->units());
    }

    Q_INVOKABLE void restore(const QString &home, const QStringList &units, const QString &snapshot)
    {
        debug::debug("Restore: home", home);
        m_vault->restore(snapshot, home, units, [this](const QString unit, const QString &status) {
            emit progress(Vault::Restore, {{"unit", unit}, {"status", status}});
        });
        emit done(Vault::Restore, QVariantMap());
    }

    Q_INVOKABLE void backup(const QString &home, const QStringList &units, const QString &message)
    {
        debug::debug("Backup: home", home);
        m_vault->backup(home, units, message, [this](const QString unit, const QString &status) {
            emit progress(Vault::Backup, {{"unit", unit}, {"status", status}});
        });
        emit done(Vault::Backup, QVariantMap());
    }

    QStringList snapshots() const
    {
        auto snapshots = m_vault->snapshots();
        QStringList snaps;
        for (const vault::Snapshot &ss: snapshots) {
            snaps << ss.name();
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
            m_transfer->init(m_vault, ac, path);
            QVariantMap data;
            data["action"] = action ==Vault:: Import ? "import" : "export";
            data["src"] = m_transfer->getSrc();
            emit done(Vault::ExportImportPrepare, data);
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
            emit done(Vault::ExportImportExecute, QVariantMap());
        } catch (error::Error e) {
            emit error(Vault::ExportImportExecute, e.what());
        }
    }

    Q_INVOKABLE void rmSnapshot(const QString &name)
    {
        debug::debug("Requesting snapshot removal:", name);
        for (vault::Snapshot ss: m_vault->snapshots()) {
            if (ss.name() == name)  {
                ss.remove();
                break;
            }
        }

        if (snapshots().size() == 0) {
            // TODO This action should be done only until proper
            // garbage collection will be introduced
            debug::warning("Last snapshot is removed, destroy storage to conserve space");
            QString root = m_vault->root();
            destroy();
            try {
                init(root);
            } catch (error::Error e) {
                debug::error("Error reconnecting", e.what());
                emit error(Vault::RemoveSnapshot, e.what());
            }
        } else {
            debug::debug("There are some snapshots, continue");
        }
        emit done(Vault::RemoveSnapshot, QVariantMap());
    }

signals:
    void progress(Vault::Operation op, const QVariantMap &map);
    void error(Vault::Operation op, const QString &error);
    void done(Vault::Operation op, const QVariantMap &);

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
        emit done(Vault::Connect, QVariantMap());
    } catch (error::Error e) {
        emit error(Vault::Connect, e.what());
    }
}

void Vault::connectVault(bool reconnect)
{
    if (m_worker && !reconnect) {
        emit done(Vault::Connect, QVariantMap());
        return;
    }

    initWorker(reconnect);
}

void Vault::startRestore(const QString &snapshot, const QStringList &units)
{
    if (units.isEmpty()) {
        debug::info("Nothing to restore");
        emit done(Vault::Restore, QVariantMap());
        return;
    }

    QMetaObject::invokeMethod(m_worker, "restore", Q_ARG(QString, m_home), Q_ARG(QStringList, units), Q_ARG(QString, snapshot));
}

void Vault::startBackup(const QString &message, const QStringList &units)
{
    if (units.isEmpty()) {
        debug::info("Nothing to backup");
        emit done(Vault::Backup, QVariantMap());
        return;
    }

    QMetaObject::invokeMethod(m_worker, "backup", Q_ARG(QString, m_home), Q_ARG(QStringList, units), Q_ARG(QString, message));
}

QStringList Vault::snapshots() const
{
    return m_worker->snapshots();
}

QVariantMap Vault::units() const
{
    QVariantMap units;
    for (auto &u: m_worker->m_vault->config().units()) {
        units.insert(u.name(), u.data());
    }
    return units;
}

void Vault::resetHead()
{
    m_worker->m_vault->reset();
}

void Vault::removeSnapshot(const QString &name)
{
    QMetaObject::invokeMethod(m_worker, "rmSnapshot", Q_ARG(QString, name));
}

void Vault::exportImportPrepare(ImportExportAction action, const QString &path)
{
    QMetaObject::invokeMethod(m_worker, "eiPrepare", Q_ARG(Vault::ImportExportAction, action), Q_ARG(QString, path));
}

void Vault::exportImportExecute()
{
    QMetaObject::invokeMethod(m_worker, "eiExecute");
}

QString Vault::notes(const QString &snapshot) const
{
    return m_worker->m_vault->notes(">" + snapshot);
}

void Vault::registerUnit(const QJSValue &unit, bool global)
{
    QVariantMap map = unit.toVariant().toMap();
    if (global) {
        vault::config::global()->set(map);
    } else {
        if (!m_worker) {
            initWorker(false);
        }
        m_worker->m_vault->registerConfig(map);
    }
}

#include "vault.moc"
