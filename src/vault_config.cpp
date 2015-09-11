/**
 * @file config.cpp
 * @brief Vault configuration
 * @author Giulio Camuffo <giulio.camuffo@jollamobile.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <QDir>

#include <gittin/repo.hpp>
#include <gittin/repostatus.hpp>
#include <gittin/commit.hpp>

#include <vault/config.hpp>
#include <qtaround/error.hpp>
#include <qtaround/os.hpp>
#include <qtaround/debug.hpp>
#include <qtaround/json.hpp>

namespace os = qtaround::os;
namespace error = qtaround::error;
namespace json = qtaround::json;
namespace debug = qtaround::debug;

namespace vault { namespace config {

Unit::Unit()
{

}

Unit::Unit(const QVariantMap &data)
    : m_data(data)
{
    m_data["is_unit_config"] = true;
    if (!data.isEmpty()) {
        update(data);
    }
}

Unit &Unit::read(const QString &fname)
{
    update(json::read(fname).toVariantMap());
    return *this;
}

ssize_t Unit::write(const QString &fname)
{
    return json::write(m_data, fname);
}

bool Unit::update(const QVariantMap &data)
{
    QVariantMap src = data;
    bool updated = false;
    if (!src.value("is_unit_config").toBool()) {
        if (!src.contains("name") || !src.contains("script")) {
            error::raise({{"msg", "Unit description should contain name and script"}});
        }
        auto script = str(src["script"]);
        auto script_path = os::path::canonical(script);
        if (!script_path.isEmpty()) {
            debug::debug("Full script path:", script_path);
            script = script_path;
        }
        debug::info("Script:", script);
        src["script"] = script;
    }
    for (auto i = src.begin(); i != src.end(); ++i) {
        if (!m_data.contains(i.key()) || i.value() != m_data.value(i.key())) {
            m_data[i.key()] = i.value();
            updated = true;
        }
    }
    return updated;
}

QString Unit::name() const
{
    return m_data.value("name").toString();
}

QString Unit::script() const
{
    return m_data.value("script").toString();
}



static Config mkGlobal()
{
    static const char *envName = "VAULT_GLOBAL_CONFIG_DIR";
    QString unitsDir = qEnvironmentVariableIsSet(envName) ? qgetenv(envName) : "/var/lib/the-vault";
    return Config(unitsDir);
}

static const char *moduleExt = ".json";
static Config s_global = mkGlobal();

Config::Config(const QString &unitsDir)
      : m_unitsDir(unitsDir)
{
    if (unitsDir.isEmpty()) {
        error::raise({{"msg", "Empty configuration path"}});
    }

    load();
}

Config::~Config()
{
}

Config *global()
{
    return &s_global;
}

void Config::load()
{
    if (!os::path::exists(m_unitsDir)) {
        return;
    }

    QDir d(m_unitsDir);
    for (const QString &fname: d.entryList({ QLatin1String("*") + moduleExt })) {
        try {
            Unit unit;
            unit.read(d.filePath(fname));
            m_units[unit.name()] = unit;
        } catch (error::Error e) {
            debug::error("Loading config ", fname);
            debug::error("Error", e.what());
        }
    }
}

bool Config::set(const QVariantMap &data)
{
    bool updated = false;
    Unit unit(data);

    QString name = unit.name();
    QString configPath = path(name);

    if (!os::path::exists(m_unitsDir)) {
        os::mkdir(m_unitsDir);
        m_units[name] = unit;
        updated = true;
    } else if (m_units.contains(name)) {
        updated = m_units[name].update(data);
    } else if (os::path::exists(configPath)) {
        Unit actual = Unit().read(configPath);
        updated = actual.update(data);
        m_units[name] = actual;
    } else {
        m_units[name] = unit;
        updated = true;
    }

    return updated && m_units[name].write(configPath);
}

QString Config::rm(const QString &name)
{
    QString fname = path(name);
    if (!os::path::exists(fname)) {
        return QString();
    }
    os::rm(fname);
    m_units.remove(name);
    return name + moduleExt;
}

void Config::setUnitsDir(const QString &dir)
{
    m_unitsDir = dir;
    if (dir.isEmpty()) {
        error::raise({{"msg", "Wrong configuration"}, {"cfg", dir}});
    }
}

QString Config::path(const QString &fname) const
{
    return m_unitsDir + "/" + fname + moduleExt;
}

QMap<QString, Unit> Config::units() const
{
    return m_units;
}

QString Config::root() const
{
    return m_unitsDir;
}



Vault::Vault(Gittin::Repo *vcs)
    : m_config(units_path(vcs->path()))
     , m_vcs(vcs)
{

}

Vault::~Vault()
{
}

bool Vault::set(const QVariantMap &data)
{
    return m_config.set(data);
}

bool Vault::rm(const QString &name)
{
    auto fname = m_config.rm(name);
    return !fname.isEmpty();
}

bool Vault::update(const QMap<QString, Unit> &src)
{
    QVariantMap map;
    for (auto i = src.begin(); i != src.end(); ++i) {
        map[i.key()] = i.value().data();
    }
    return update(map);
}

bool Vault::update(const QVariantMap &src)
{
    bool updated = false;
    auto before = m_config.units();
    for (auto i = src.begin(); i != src.end(); ++i) {
        if (set(i.value().toMap())) {
            updated = true;
        }
    }
    // remove local copy of the global unit if there is no global
    // counterpart
    for (auto it = before.cbegin(); it != before.cend(); ++it) {
        auto name = it.key();
        if (it.value().isLocal() || src.contains(name))
            continue;

        debug::info("Global unit", name, "is not registered, removing");
        if (!rm(name))
            error::raise({{"msg", name + " is not removed??"}});

        updated = true;
    }
    return updated;
}

QMap<QString, Unit> Vault::units() const
{
    return m_config.units();
}

}}
