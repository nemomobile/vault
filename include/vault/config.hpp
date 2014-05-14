#ifndef _VAULT_CONFIG_HPP_
#define _VAULT_CONFIG_HPP_

#include <QString>
#include <QMap>
#include <QVariantMap>

namespace Gittin {
class Repo;
}

namespace vault { namespace config {

namespace {

const QString prefix = ".f8b52b7481393a3e6ade051ecfb549fa";
}

class Unit
{
public:
    Unit();
    explicit Unit(const QVariantMap &data);

    Unit &read(const QString &fname);
    ssize_t write(const QString &fname);
    bool update(const QVariantMap &src);

    QString name() const;
    QString script() const;
    inline QVariantMap data() const { return m_data; }

private:
    QVariantMap m_data;
};

class Config
{
public:
    explicit Config(const QString &unitsDir);
    ~Config();

    void load();
    bool set(const QVariantMap &data);
    QString rm(const QString &name);

    void setUnitsDir(const QString &unitsDir);

    QMap<QString, Unit> units() const;
    QString path(const QString &fname) const;
    QString root() const;

    static Config *global();

private:
    QString m_unitsDir;
    QMap<QString, Unit> m_units;
    static Config s_global;
};

class Vault
{
public:
    explicit Vault(Gittin::Repo *vcs);
    ~Vault();

    bool set(const QVariantMap &data);
    bool rm(const QString &name);
    bool update(const QMap<QString, Unit> &src);
    bool update(const QVariantMap &src);

    QMap<QString, Unit> units() const;

private:
    Config m_config;
    Gittin::Repo *m_vcs;
};

}}

#endif // _VAULT_CONFIG_HPP_
