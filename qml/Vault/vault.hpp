
#ifndef QML_VAULT_H
#define QML_VAULT_H

#include <QObject>
#include <QThread>
#include <QStringList>

class Worker;

class Vault : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString root READ root WRITE setRoot NOTIFY rootChanged)
    Q_PROPERTY(QString backupHome READ backupHome WRITE setBackupHome NOTIFY backupHomeChanged)
public:
    explicit Vault(QObject *parent = nullptr);
    ~Vault();

    QString root() const;
    QString backupHome() const;

    void setRoot(const QString &root);
    void setBackupHome(const QString &home);

    Q_INVOKABLE void connectVault(bool reconnect);
    Q_INVOKABLE void startBackup(const QString &message, const QStringList &units);
    Q_INVOKABLE void startRestore(const QString &tag, const QStringList &units);
    Q_INVOKABLE QStringList snapshots() const;
    Q_INVOKABLE QStringList units() const;
    Q_INVOKABLE void resetHead();
    Q_INVOKABLE void removeSnapshot(const QString &name);
    Q_INVOKABLE void exportImportPrepare();

signals:
    void rootChanged();
    void backupHomeChanged();

    void connectDone();
    void backupDone();
    void restoreDone();
    void progress(const QString &unit, const QString &status);
    void error(const QString &error);

private:
    void initWorker(bool reload);

    QThread m_workerThread;
    Worker *m_worker;
    QString m_home;
    QString m_root;
};

#endif
