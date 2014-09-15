#include <transfer.hpp>
#include "vault_context.hpp"
#include "tests_common.hpp"

#include <qtaround/debug.hpp>
#include <qtaround/util.hpp>
#include <qtaround/sys.hpp>
#include <qtaround/os.hpp>
#include <qtaround/subprocess.hpp>

#include <vault/config.hpp>
#include <vault/vault.hpp>

#include <cor/util.hpp>
#include <tut/tut.hpp>

//#include "tests_common.hpp"

#include <QDebug>
#include <QRegExp>

#include <iostream>
#include <unistd.h>

namespace subprocess = qtaround::subprocess;

namespace tut
{

struct transfer_test
{
    virtual ~transfer_test()
    {
    }
};

typedef test_group<transfer_test> tf;
typedef tf::object object;
tf vault_transfer_test("transfer");

enum test_ids {
    tid_setup = 1
    , tid_export
    , tid_import
};

namespace {

using vault::Vault;
std::unique_ptr<Vault> the_vault;

void register_unit(QString const &vault_dir, QString const &name)
{
    QVariantMap data = {{"action", "register"},
                        {"data", "name=" + name + ",group=group1,"
                         + "script=./" + name},
                        {"vault", vault_dir}};
    return vault::Vault::execute(data);
};

void init_vault(QString const &vault_dir)
{
    tut::ensure("Vault dir is path", !vault_dir.isEmpty());
    the_vault.reset(new Vault(vault_dir));
    the_vault->init(map({{"user.name", "NAME"}, {"user.email", "email@domain.to"}}));
}


void create_backup()
{
    auto home = str(get(context, "home"));
    auto vault_dir = str(get(context, "vault_dir"));
    init_vault(vault_dir);
    mktree(unit1_tree, str(get(context, "unit1_dir")));
    register_unit(vault_dir, "unit1");

    bool is_started = false, is_failed = false;
    the_vault->backup(home, {}, "", [&](const QString &, const QString &status) {
        if (status == "fail")
            is_failed = true;
        else if (status == "begin")
            is_started = true;
    });
    tut::ensure("backup was not started", is_started);
    tut::ensure("backup is failed", !is_failed);
}

QString home;
QString vault_dir;
QString archive_dir;
QString git_dir;
QSet<QString> ftree_git_before_export;

std::unique_ptr<CardTransfer> export_ctx;
std::unique_ptr<CardTransfer> import_ctx;

QStringList stages;
void on_progress(QVariantMap &&data)
{
    tut::ensure("There should be progress report type"
                , data.find("type") != data.end());
    auto t = str(data["type"]);
    if (t == "stage") {
        auto stage = str(data["stage"]);
        stages.push_back(stage);
    }
}

}

template<> template<>
void object::test<tid_setup>()
{
    home = str(get(context, "home"));
    archive_dir = os::path::join(str(get(context, "home")), "sd");
    os::mkdir(archive_dir);
    create_backup();
}

void ensure_trees_equal(QString const &msg
                        , QSet<QString> const &before
                        , QSet<QString> const &after)
{
    auto before_wo_after = before - after;
    auto after_wo_before = after - before;
    // compare to see difference on failure
    ensure_eq(msg + ": something is appeared"
              , after_wo_before, QSet<QString>());
    ensure_eq(msg + ": something was removed"
              , before_wo_after, QSet<QString>());
}

template<> template<>
void object::test<tid_export>()
{
    vault_dir = str(get(context, "vault_dir"));
    git_dir = os::path::join(vault_dir, ".git");
    export_ctx = cor::make_unique<CardTransfer>();
    export_ctx->init(the_vault.get(), CardTransfer::Export, archive_dir);
    ftree_git_before_export = get_ftree(git_dir);

    auto dst = export_ctx->getDst(), src = export_ctx->getSrc();
    ensure_eq("Source", src, vault_dir);
    ensure_eq("Dst", os::path::dirName(dst), archive_dir);
    ensure_eq("Export Action", export_ctx->getAction()
              , CardTransfer::Export);
    auto space_before = export_ctx->getSpace();
    ensure_ge("Export Space", (int)space_before, 1);

    stages.clear();
    export_ctx->execute(on_progress);
    ensure_eq("Expected stages", stages
              , QStringList({"Copy", "Flush", "Validate"}));
    ensure(("Dst file should exist" + dst).toStdString()
           , os::path::isFile(dst));
    auto out = str(subprocess::check_output("tar", {"tf", dst}));
    auto names = filterEmpty(out.split("\n"));
    auto tag_fname = vault::fileName(vault::File::State);
    for (auto it = names.begin(); it != names.end(); ++it) {
        auto const &name = *it;
        static QRegExp const git_file_re("^\\.git/.*$"); 
        auto is_git_file = git_file_re.exactMatch(name);
        auto is_tag = (name == tag_fname);
        ensure(("Unexpected file:" + name).toStdString()
               , is_git_file || is_tag);
    }
    ensure_trees_equal("Export"
                       , ftree_git_before_export
                       , get_ftree(git_dir));
}

template<> template<>
void object::test<tid_import>()
{
    vault_dir = os::path::join(home, "vault_imported");
    git_dir = os::path::join(vault_dir, ".git");
    init_vault(vault_dir);

    import_ctx = cor::make_unique<CardTransfer>();
    import_ctx->init(the_vault.get(), CardTransfer::Import, archive_dir);

    auto dst = import_ctx->getDst(), src = import_ctx->getSrc();
    ensure_eq("Import Dst", dst, vault_dir);
    ensure_eq("Import Src", os::path::dirName(src), archive_dir);
    ensure_eq("Import Action", import_ctx->getAction()
              , CardTransfer::Import);
    auto space_before = import_ctx->getSpace();
    ensure_ge("Import Space", space_before, 1);

    stages.clear();
    import_ctx->execute(on_progress);
    ensure_eq("Expected stages", stages
              , QStringList({"Validate", "Copy"}));

    ensure_trees_equal("Import"
                       , ftree_git_before_export
                       , get_ftree(git_dir));
}


}
