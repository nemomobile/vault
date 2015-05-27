#include "vault_context.hpp"

#include <qtaround/sys.hpp>
#include <qtaround/os.hpp>
#include <qtaround/subprocess.hpp>

#include <vault/config.hpp>
#include <vault/vault.hpp>

#include <tut/tut.hpp>

//#include "tests_common.hpp"

#include <QDebug>
#include <QRegExp>

#include <iostream>
#include <unistd.h>

namespace os = qtaround::os;
namespace subprocess = qtaround::subprocess;
namespace error = qtaround::error;

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
    tid_clear,
    tid_cli_backup_restore_several_units,
    tid_backup_fail
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

// separate home for each test
QString home;
QString vault_dir = os::path::join(home, ".vault");
QString global_mod_dir = os::path::join(home, ".units");
std::unique_ptr<vault::Vault> vlt;

std::function<void ()> setup(test_ids id)
{
    reinitContext(QString::number(id));
    home = str(context["home"]);
    if (os::path::exists(home))
        error::raise({{"dir", home}, {"msg", "Shouldn't exists"}});

    if (!os::mkdir(home))
        error::raise({{"dir", home}, {"msg", "Can't create"}});
    vault_dir = os::path::join(home, ".vault");
    global_mod_dir = os::path::join(home, ".units");
    vlt.reset(new vault::Vault(vault_dir));

    vault::config::global()->setUnitsDir(global_mod_dir);
    os::rmtree(global_mod_dir);
    os::mkdir(global_mod_dir);

    return []() { os::rmtree(home); };
}

} // namespace

void vault_init()
{
    QVariantMap git_cfg = {{"user.name", "NAME"}, {"user.email", "email@domain.to"}};
    vlt->init(git_cfg);
    ensure(S_("No vault dir", vault_dir), os::path::isDir(vault_dir));
    ensure(S_("Vault doesn't exists", vault_dir), vlt->exists());
    ensure(S_("Vault is invalid", vault_dir), !vlt->isInvalid());
}

template<> template<>
void object::test<tid_init>()
{
    auto on_exit = setup(tid_init);
    vault_init();
    on_exit();
}


template<> template<>
void object::test<tid_config_global>()
{
    auto on_exit = setup(tid_config_global);
    register_unit(vault_dir, "unit1", true);
    QString unit1_fname = os::path::join(global_mod_dir, "unit1.json");
    ensure("unit 1 global config", os::path::isFile(unit1_fname));

    QMap<QString, vault::config::Unit> units = vault::config::global()->units();
    int mod_count = 0;
    ensure("Unit in config", units.contains("unit1"));
    mod_count += units.size();
    ensure("One unit/member", mod_count == 1);

    register_unit(vault_dir, "unit2", true);
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
    auto on_exit = setup(tid_config_local);
    vault_init();
    register_unit(vault_dir, "unit1", false);
    QString config_dir = vault::config::units_path(vault_dir);
    vault::config::Config config(config_dir);
    ensure("unit1 config file", os::path::isFile(config.path("unit1")));
    QMap<QString, vault::config::Unit> units = vlt->config().units();
    ensure("no unit1 in vault config", units.contains("unit1"));

    vault::Vault::execute({{"action", "unregister"}, {"vault", vault_dir}, {"unit", "unit1"}});
    units = vlt->config().units();
    ensure("no unit1 in vault config", !units.contains("unit1"));
    on_exit();
};

template<> template<>
void object::test<tid_config_update>()
{
    auto on_exit = setup(tid_config_update);
    os::rmtree(home);
    os::mkdir(home);
    vault_init();
    register_unit(vault_dir, "unit1", true);
    QMap<QString, vault::config::Unit> units = vault::config::global()->units();
    ensure("no unit1 in global config", units.contains("unit1"));

    register_unit(vault_dir, "unit2", false);
    vault::config::Vault config = vlt->config();
    units = config.units();
    ensure("no unit2 in vault config", units.contains("unit2"));
    config.update(vault::config::global()->units());

    units = vlt->config().units();
    ensure("no unit1 in vault config", units.contains("unit1"));
    ensure("local unit2 should not be removed", units.contains("unit2"));
    on_exit();
}

void do_backup()
{
    bool is_started = false, is_failed = false;
    vlt->backup(home, {}, "", [&](const QString &, const QString &status) {
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
    ensure("no snapshot", !vlt->snapshots().isEmpty());
    vlt->restore(vlt->snapshots().last(), home, {}, [&](const QString &, const QString &status) {
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
    auto on_exit = setup(tid_simple_blobs);
    os::rmtree(home);
    os::mkdir(home);
    vault_init();
    register_unit(vault_dir, "unit1", false);
    QMap<QString, vault::config::Unit> units = vlt->config().units();
    ensure("no unit1 in vault config", units.contains("unit1"));

    auto unit1_dir = str(get(context, "unit1_dir"));
    mktree(unit1_tree, unit1_dir);
    auto ftree_git_before_export = get_ftree(unit1_dir);

    ensure("No snapshots yet", vlt->snapshots().length() == 0);
    do_backup();
    ensure("Added 1 snapshot", vlt->snapshots().length() == 1);
    auto ftree_git_after_export = get_ftree(unit1_dir);
    ensure_trees_equal("Src is modified?"
                       , ftree_git_before_export
                       , get_ftree(unit1_dir));

    vault::Vault::UnitPath unit1_vault = vlt->unitPath("unit1");
    QString unit1_vault_path = os::path::canonical(unit1_vault.bin);
    ensure(S_("dir is not created", unit1_vault_path)
           , os::path::isDir(unit1_vault_path));
    QString unit1_vault_blob = os::path::join(unit1_vault_path, "unit1"
                                              , "binaries", "b1");
    ensure(S_("should be symlink", unit1_vault_blob)
           , os::path::isSymLink(unit1_vault_blob));

    os::rmtree(unit1_dir);
    ensure(AT, !os::path::exists(unit1_dir));
    do_restore();
    ensure_trees_equal("Src is not restored?"
                       , ftree_git_before_export
                       , get_ftree(unit1_dir));

    on_exit();
}

template<> template<>
void object::test<tid_clear>()
{
    auto on_exit = setup(tid_clear);
    os::rmtree(home);

    // valid vault
    os::mkdir(home);
    vault_init();
    ensure("No vault before", os::path::exists(vlt->root()));
    ensure("W/o params do nothing", !vlt->clear(QVariantMap()));
    ensure("Vault should not be removed", os::path::exists(vlt->root()));
    ensure("Should destroy valid valut", vlt->clear({{"destroy", true}}));
    ensure("Vault should be removed", !os::path::exists(vlt->root()));

    // invalid vault
    ensure(os::mkdir(vault_dir));
    ensure("Should not destroy invalid vault", !vlt->clear({{"destroy", true}}));
    ensure("Invalid vault should not be removed", os::path::exists(vlt->root()));
    ensure("Should destroy invalid vault", vlt->clear({{"destroy", true}, {"clear_invalid", true}}));
    ensure("Invalid vault should be removed", !os::path::exists(vlt->root()));

    // vault with snapshots
    os::mkdir(home);
    vault_init();
    register_unit(vault_dir, "unit1", false);

    QString unit1_dir = os::path::join(home, "unit1");
    os::mkdir(unit1_dir);
    QString unit1_blob = os::path::join(unit1_dir, "blob1");
    os::write_file(unit1_blob, "1\n2\n3\n");
    ensure("No snapshots yet", vlt->snapshots().length() == 0);
    do_backup();
    ensure("Added 1 snapshot", vlt->snapshots().length() == 1);
    ensure("Should not destroy vault with snapshots", !vlt->clear({{"destroy", true}}));
    ensure("Should destroy vault with snapshots", vlt->clear({{"destroy", true}, {"ignore_snapshots", true}}));
    ensure("Vault should be removed", !os::path::exists(vlt->root()));

    on_exit();
}


template<> template<>
void object::test<tid_cli_backup_restore_several_units>()
{
    auto on_exit = setup(tid_cli_backup_restore_several_units);
    os::rmtree(home);
    os::mkdir(home);
    vault_init();
    register_unit(vault_dir, "unit1", false);
    register_unit(vault_dir, "unit2", false);
    auto rc = vault::Vault::execute({{"action", "export"}
            , {"vault", vault_dir}
            , {"home", home}
            , {"unit", "unit1,unit2"}});
    ensure_eq("Should succeed", rc, 0);
    on_exit();
}

template<> template<>
void object::test<tid_backup_fail>()
{
    using vault::Vault;
    auto on_exit = setup(tid_backup_fail);
    auto describe = []() {
        subprocess::Process ps;
        ps.setWorkingDirectory(vault_dir);
        ps.start("git", {"describe", "--tags", "--exact-match"});
        return std::move(ps);
    };
    auto last_snapshot = []() {
        return vlt->snapshots().last().name();
    };
    os::rmtree(home);
    os::mkdir(home);
    vault_init();
    register_unit(vault_dir, "unit1", false);
    register_unit(vault_dir, "unit2", false);
    auto rc = Vault::execute({{"action", "export"}
            , {"vault", vault_dir}
            , {"home", home}
            , {"unit", "unit1,unit2"}});
    ensure_eq("These units should proceed", rc, 0);

    auto snap1 = last_snapshot();

    register_unit(vault_dir, "unit_fail", false);
    rc = Vault::execute({{"action", "export"}
            , {"vault", vault_dir}
            , {"home", home}
            , {"unit", "unit1,unit2,unit_fail"}});
    ensure_eq("With failed unit it should fail", rc, 1);

    auto ps = describe();
    ensure("Git is not found", ps.wait(10));
    ensure_eq("There should be a previous tag on top", ps.rc(), 0);
    ensure_eq("Should be the same snapshot as before", snap1, last_snapshot());
    on_exit();
}

}
