#include <transfer.hpp>
#include "vault_context.hpp"

#include "util.hpp"
#include "sys.hpp"
#include "os.hpp"
#include "subprocess.hpp"
#include <vault/config.hpp>
#include <vault/vault.hpp>

#include <tut/tut.hpp>

//#include "tests_common.hpp"

#include <QDebug>
#include <QRegExp>

#include <iostream>
#include <unistd.h>

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
    , tid_prepare
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

void create_backup()
{
    auto home = str(get(context, "home"));
    auto vault_dir = str(get(context, "vault_dir"));
    tut::ensure("Vault dir", !vault_dir.isEmpty());
    the_vault.reset(new Vault(vault_dir));
    the_vault->init(map({{"user.name", "NAME"}, {"user.email", "email@domain.to"}}));
    mktree(unit1_tree, str(get(context, "unit1_dir")));
    register_unit(vault_dir, "unit1");
}

}

template<> template<>
void object::test<tid_setup>()
{
    setup_context();
}

template<> template<>
void object::test<tid_prepare>()
{
    auto home = str(get(context, "home"));
    create_backup();
    // auto is_failed = false, err, export_ctx, tgt_path, rc = -1, import_ctx;
    auto tgt_path = os::path::join(str(get(context, "home")), "sd");
    os::mkdir(tgt_path);

    auto git_dir = os::path::join(str(get(context, "vault_dir")), ".git");
    auto ftree_git_before_export = get_ftree(git_dir);
    auto ctx = cor::make_unique<CardTransfer>();
    ctx->init(the_vault.get(), CardTransfer::Export, tgt_path);

    // test.equal(export_ctx.src, context.vault_dir);
    // test.equal(os::path.dirname(export_ctx.dst), tgt_path);
    // test.equal(export_ctx.action, "export");
    // test.ge(export_ctx.space_free, 1);

    // api.export_import_execute
    // (export_ctx
    //  , {{"on_done", function}(ret_rc) {
    //          rc = ret_rc;
    //          }
    //     , {"on_error", function}(e) {
    //         is_failed = true;
    //         err = e;
    //     }});
    // api.wait();

    // test.ok(!is_failed, "Failed because " + util.dump("ERR", err));
    // test.ok(os::path.isFile(export_ctx.dst));
    // var content = subprocess.check_output
    //     ("tar", ["tf", export_ctx.dst]).toString().split("\n");
    // content = string.removeEmpty(content);
    // var tag_fname = vault.info.files.state;
    // util.map(content, function(line) {
    //     test.ok(/^\.git/.test(line) || line === tag_fname
    //             , "Unexpected file:" + line);
    // });
    // var ftree_git_after_export = get_ftree(git_dir);
    // var before_diff_after
    //     = _.difference(ftree_git_before_export, ftree_git_after_export);
    // var after_diff_before
    //     = _.difference(ftree_git_after_export, ftree_git_before_export);
    // test.deepEqual(before_diff_after, []);
    // test.deepEqual(after_diff_before, []);

    // // import

    // var test_import = function() {
    //     import_ctx = undefined;
    //     api.export_import_prepare
    //     ({{"action", "import"}, {"path", tgt_path}}
    //      , {{"on_done", function}(ctx) {
    //              import_ctx = ctx;
    //              },
    //         {"on_error", function}(e) {
    //             is_failed = true;
    //             err = e;
    //         }});
    //     api.wait();
    //     test.ok(!is_failed, "Failed because " + util.dump("ERR", err));
    //     test.ok(typeof import_ctx === "object");
    //     test.equal(import_ctx.dst, context.vault_dir);
    //     test.equal(os::path.dirname(import_ctx.src), tgt_path);
    //     test.equal(import_ctx.action, "import");
    //     test.ge(import_ctx.space_free, 1);

    //     api.export_import_execute
    //     (import_ctx
    //      , { {"on_done", function}(ret_rc) {
    //          rc = ret_rc;
    //      }, {"on_error", function}(e) {
    //          is_failed = true;
    //          err = e;
    //      }});
    //     api.wait();

    //     test.ok(!is_failed, "Failed because " + util.dump("ERR", err));
    //     var ftree_git_after_import = get_ftree(git_dir);
    //     test.deepEqual(ftree_git_before_export, ftree_git_after_import);
    // };
    // test_import();
    // os::rmtree(context.vault_dir);
    // test.ok(!os::path.exists(context.vault_dir));
    // api.connect({{"home", home}, {"vault", vault_dir}});
    // test_import();
}


}
