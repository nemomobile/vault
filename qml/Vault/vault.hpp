
#ifndef QML_VAULT_H
#define QML_VAULT_H

#include <QObject>
#include <QThread>
#include <QStringList>
#include <QVariantMap>

class QJSValue;
class Worker;

class Vault : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString root READ root WRITE setRoot NOTIFY rootChanged)
    Q_PROPERTY(QString backupHome READ backupHome WRITE setBackupHome NOTIFY backupHomeChanged)
public:
    enum ImportExportAction {
        Export,
        Import
    };
    Q_ENUMS(ImportExportAction);
    enum Operation {
        Connect,
        Backup,
        Restore,
        RemoveSnapshot,
        ExportImportPrepare,
        ExportImportExecute,
        Data,
        Maintenance,
        ExportSnapshot
    }
    Q_ENUMS(Operation);

    enum DataType {
        SnapshotUnits
    }
    Q_ENUMS(DataType);

    explicit Vault(QObject *parent = nullptr);
    ~Vault();

    QString root() const;
    QString backupHome() const;

    void setRoot(const QString &root);
    void setBackupHome(const QString &home);

    Q_INVOKABLE void connectVault(bool reconnect);
    Q_INVOKABLE void startBackup(const QString &message, const QStringList &units);
    Q_INVOKABLE void startRestore(const QString &snapshot, const QStringList &units);
    Q_INVOKABLE QStringList snapshots() const;
    Q_INVOKABLE QVariantMap units() const;
    Q_INVOKABLE void resetHead();
    Q_INVOKABLE void removeSnapshot(const QString &name);
    Q_INVOKABLE void exportImportPrepare(ImportExportAction action, const QString &path);
    Q_INVOKABLE void exportImportExecute();
    Q_INVOKABLE QString notes(const QString &snapshot) const;

    Q_INVOKABLE void registerUnit(const QJSValue &unit, bool global);
    Q_INVOKABLE void startGc();
    Q_INVOKABLE void requestData(DataType, QVariantMap const &);

    Q_INVOKABLE void reset();
    Q_INVOKABLE void backupUnit(const QString &unit);
    Q_INVOKABLE void tagSnapshot(const QString &message);

    Q_INVOKABLE void restoreUnit(const QString &, const QString &);
    Q_INVOKABLE void exportSnapshot(const QString &, const QString &);

signals:
    void rootChanged();
    void backupHomeChanged();

    void done(Operation operation, const QVariantMap &data);
    void progress(Operation operation, const QVariantMap &data);
    void error(Operation operation, const QVariantMap &error);

    void data(DataType id, const QVariantMap &context);

private:
    void initWorker(bool reload);

    QThread m_workerThread;
    Worker *m_worker;
    QString m_home;
    QString m_root;
};

#endif
