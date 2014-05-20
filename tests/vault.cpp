#include <qtaround/sys.hpp>
#include <qtaround/os.hpp>
#include <qtaround/subprocess.hpp>

#include <vault/config.hpp>
#include <vault/vault.hpp>

#include "tests_common.hpp"
#include <tut/tut.hpp>

//#include "tests_common.hpp"

#include <QDebug>
#include <QRegExp>

#include <iostream>
#include <unistd.h>

namespace tut
{

struct unit_vault
{
    virtual ~unit_vault()
    {
    }
};

typedef test_group<unit_vault> tf;
typedef tf::object object;
tf vault_unit_vault("vault");

enum test_ids {
    tid_init =  1,
    tid_config_global,
    tid_config_local,
    tid_config_update,
    tid_simple_blobs,
    tid_clear
};

namespace {

const string_map_type short_options = {
};

const string_map_type long_options = {
    {"bin-dir", "bin-dir"}, {"dir", "dir"}
    , {"home-dir", "home-dir"}, {"action", "action"}
};

const QSet<QString> options_has_param = {
    {"bin-dir", "dir", "home-dir", "action"}
};

const QString home = "/tmp/.test-the-vault-431f7f8b665d3cc2a1a4a8c3c74ad5ff";
const QString vault_dir = os::path::join(home, ".vault");
const QString global_mod_dir = os::path::join(home, ".units");
vault::Vault vlt(vault_dir);

std::function<void ()> setup()
{
    if (os::path::exists(home)) {
        error::raise({{"dir", home}, {"msg", "should not exist"}});
    }
    os::mkdir(home);

    vault::config::global()->setUnitsDir(global_mod_dir);
    os::rmtree(global_mod_dir);
    os::mkdir(global_mod_dir);

    return []() { os::rmtree(home); };
}

} // namespace

void vault_init()
{
    QVariantMap git_cfg = {{"user.name", "NAME"}, {"user.email", "email@domain.to"}};
    vlt.init(git_cfg);
    ensure("Vault is created", os::path::isDir(vault_dir));
    ensure("Vault exists", vlt.exists());
    ensure("Vault is invalid", !vlt.isInvalid());
}

void register_unit(const QString &name, bool is_global)
{
    QVariantMap data = {{"action", "register"},
                        {"data", "name=" + name + ",group=group1,"
                                 + "script=./" + name + "_vault_test"}};
    if (is_global) {
        data["global"] = true;
    } else {
        data["vault"] = vault_dir;
    }
    return vault::Vault::execute(data);
};

template<> template<>
void object::test<tid_init>()
{
    auto on_exit = setup();
    vault_init();
    on_exit();
}


template<> template<>
void object::test<tid_config_global>()
{
    auto on_exit = setup();
    register_unit("unit1", true);
    QString unit1_fname = os::path::join(global_mod_dir, "unit1.json");
    ensure("unit 1 global config", os::path::isFile(unit1_fname));

    QMap<QString, vault::config::Unit> units = vault::config::global()->units();
    int mod_count = 0;
    ensure("Unit in config", units.contains("unit1"));
    mod_count += units.size();
    ensure("One unit/member", mod_count == 1);

    register_unit("unit2", true);
    units = vault::config::global()->units();
    mod_count = 0;
    QString unit2_fname = os::path::join(global_mod_dir, "unit2.json");
    ensure("unit 2 global config", os::path::isFile(unit2_fname));
    mod_count += units.size();
    ensure("Unit2 in config", units.contains("unit2"));
    ensure("One unit/member", mod_count == 2);
    ensure("unit 1 global config", os::path::isFile(unit1_fname));

    vault::Vault::execute({{"action", "unregister"},  {"global", true}, {"unit", "unit1"}});
    vault::Vault::execute({{"action", "unregister"},  {"global", true}, {"unit", "unit2"}});
    units = vault::config::global()->units();
    ensure("global config is removed", !os::path::exists(unit1_fname));
    ensure("no unit1 in global config", !units.contains("unit1"));
    ensure("no unit2 in global config", !units.contains("unit2"));
    on_exit();
}

template<> template<>
void object::test<tid_config_local>()
{
    auto on_exit = setup();
    vault_init();
    register_unit("unit1", false);
    QString config_dir = os::path::join(vault_dir, ".modules");
    vault::config::Config config(config_dir);
    ensure("unit1 config file", os::path::isFile(config.path("unit1")));
    QMap<QString, vault::config::Unit> units = vlt.config().units();
    ensure("no unit1 in vault config", units.contains("unit1"));

    vault::Vault::execute({{"action", "unregister"}, {"vault", vault_dir}, {"unit", "unit1"}});
    units = vlt.config().units();
    ensure("no unit1 in vault config", !units.contains("unit1"));
    on_exit();
};

template<> template<>
void object::test<tid_config_update>()
{
    auto on_exit = setup();
    os::rmtree(home);
    os::mkdir(home);
    vault_init();
    register_unit("unit1", true);
    QMap<QString, vault::config::Unit> units = vault::config::global()->units();
    ensure("no unit1 in global config", units.contains("unit1"));

    register_unit("unit2", false);
    vault::config::Vault config = vlt.config();
    units = config.units();
    ensure("no unit2 in vault config", units.contains("unit2"));
    config.update(vault::config::global()->units());

    units = vlt.config().units();
    ensure("no unit1 in vault config", units.contains("unit1"));
    ensure("unit2 should be removed from vault config", !units.contains("unit2"));
    on_exit();
}

void do_backup()
{
    bool is_started = false, is_failed = false;
    vlt.backup(home, {}, "", [&](const QString &, const QString &status) {
        if (status == "fail")
            is_failed = true;
        else if (status == "begin")
            is_started = true;
    });
    ensure("backup was not started", is_started);
    ensure("backup is failed", !is_failed);
}

void do_restore()
{
    bool is_started = false, is_failed = false;
    ensure("no snapshot", !vlt.snapshots().isEmpty());
    vlt.restore(vlt.snapshots().last(), home, {}, [&](const QString &, const QString &status) {
        if (status == "fail")
            is_failed = true;
        else if (status == "begin")
            is_started = true;
    });
    ensure("backup was not started", is_started);
    ensure("backup is failed", !is_failed);
}

template<> template<>
void object::test<tid_simple_blobs>()
{
    auto on_exit = setup();
    os::rmtree(home);
    os::mkdir(home);
    vault_init();
    register_unit("unit1", false);
    QMap<QString, vault::config::Unit> units = vlt.config().units();
    ensure("no unit1 in vault config", units.contains("unit1"));

    QString unit1_dir = os::path::join(home, "unit1");
    os::mkdir(unit1_dir);
    QString unit1_blob = os::path::join(unit1_dir, "blob1");
    os::write_file(unit1_blob, "1\n2\n3\n");
    ensure("unit1 is prepared", os::path::isDir(unit1_dir));
    ensure("unit1 blob is prepared", os::path::isFile(unit1_blob));

    ensure("No snapshots yet", vlt.snapshots().length() == 0);
    do_backup();
    ensure("Added 1 snapshot", vlt.snapshots().length() == 1);
    ensure("unit1 is still here", os::path::isDir(unit1_dir));
    ensure("unit1 blob is still here", os::path::isFile(unit1_blob));
    vault::Vault::UnitPath unit1_vault = vlt.unitPath("unit1");
    QString unit1_vault_path = QFileInfo(unit1_vault.bin).absoluteFilePath();
    ensure("unit1 is in the vault", os::path::isDir(unit1_vault_path));
    QString unit1_vault_blob = os::path::join(unit1_vault_path, "unit1", "blob1");
    ensure("unit1 blob1 is in the vault", os::path::exists(unit1_vault_blob));
    ensure("unit1 blob1 is symlink", os::path::isSymLink(unit1_vault_blob));

    os::rm(unit1_blob);
    ensure("unit1 blob is still here", !os::path::isFile(unit1_blob));
    do_restore();
    ensure("unit1 blob not restored", os::path::isFile(unit1_blob));

    on_exit();
}

template<> template<>
void object::test<tid_clear>()
{
    auto on_exit = setup();
    os::rmtree(home);

    // valid vault
    os::mkdir(home);
    vault_init();
    ensure("No vault before", os::path::exists(vlt.root()));
    ensure("W/o params do nothing", !vlt.clear(QVariantMap()));
    ensure("Vault should not be removed", os::path::exists(vlt.root()));
    ensure("Should destroy valid valut", vlt.clear({{"destroy", true}}));
    ensure("Vault should be removed", !os::path::exists(vlt.root()));

    // invalid vault
    ensure(os::mkdir(vault_dir));
    ensure("Should not destroy invalid vault", !vlt.clear({{"destroy", true}}));
    ensure("Invalid vault should not be removed", os::path::exists(vlt.root()));
    ensure("Should destroy invalid vault", vlt.clear({{"destroy", true}, {"clear_invalid", true}}));
    ensure("Invalid vault should be removed", !os::path::exists(vlt.root()));

    // vault with snapshots
    os::mkdir(home);
    vault_init();
    register_unit("unit1", false);

    QString unit1_dir = os::path::join(home, "unit1");
    os::mkdir(unit1_dir);
    QString unit1_blob = os::path::join(unit1_dir, "blob1");
    os::write_file(unit1_blob, "1\n2\n3\n");
    ensure("No snapshots yet", vlt.snapshots().length() == 0);
    do_backup();
    ensure("Added 1 snapshot", vlt.snapshots().length() == 1);
    ensure("Should not destroy vault with snapshots", !vlt.clear({{"destroy", true}}));
    ensure("Should destroy vault with snapshots", vlt.clear({{"destroy", true}, {"ignore_snapshots", true}}));
    ensure("Vault should be removed", !os::path::exists(vlt.root()));

    on_exit();
}

}
