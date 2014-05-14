#include "sys.hpp"
#include "os.hpp"
#include "subprocess.hpp"
#include <vault/config.hpp>

#include <tut/tut.hpp>

//#include "tests_common.hpp"

#include <QDebug>
#include <QRegExp>

#include <iostream>
#include <unistd.h>

namespace tut
{

struct unit_test
{
    virtual ~unit_test()
    {
    }
};

typedef test_group<unit_test> tf;
typedef tf::object object;
tf vault_unit_test("unit");

enum test_ids {
    tid_export =  1,
    tid_import
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

const QString root = "/tmp/test-the-vault-unit";
const QString home = os::path::join(root, "home");
const QString vault = os::path::join(root, "vault");
const QString home_out = os::path::join(root, "home_out");

typedef cor::ScopeExit<std::function<void ()> > teardown_type;

teardown_type setup()
{
    auto mkdir = [](QString const &p) {
        ensure(std::string("Dir should not exist:") + p.toStdString()
               , !os::path::exists(p));
        if (!os::mkdir(p, {{"parent", true}}))
            error::raise({{"msg", "Can't create"}, {"path", p}});
    };
    mkdir(home);
    mkdir(home_out);
    mkdir(vault);
    teardown_type on_exit([]() {
            os::rmtree(root);
        });

    QMap<QString, QString> home_dirs = {{ "data", os::path::join(home, "data")}
                                        , {"bin", os::path::join(home, "bin")}};
    for (auto it = home_dirs.begin(); it != home_dirs.end(); ++it) {
        auto t = it.key();
        auto root_path = it.value();

        mkdir(root_path);
        using os::path;
        auto dir_content = path::join(root_path, "content");
        mkdir(dir_content);
        mkdir(path::join(dir_content, "content_subdir"));
        for (int i = 0; i < 3; ++i) {
            QString s = str(i);
            os::write_file(path::join(dir_content, "a" + s), s);
        }

        auto dir_self = path::join(root_path, ".hidden_dir_self");
        mkdir(dir_self);
        for (int i = 0; i < 4; ++i) {
            QString s = str(i);
            os::write_file(path::join(dir_self, "b" + s), s);
        }

        os::write_file(path::join(root_path, "file1"), "c1");
        auto in_dir = path::join(root_path, "in_dir");
        mkdir(in_dir);
        os::write_file(path::join(in_dir, "file2"), "d2");

        auto linked_dir = path::join(root_path, "linked_dir");
        mkdir(linked_dir);
        os::write_file(path::join(linked_dir, "e1"), "1");
        os::symlink("./linked_dir", path::join(root_path, "symlink_to_dir"));

    }

    return std::move(on_exit);
}

} // namespace

template <class CharT>
std::basic_ostream<CharT>& operator <<
(std::basic_ostream<CharT> &dst, QStringList const &src)
{
    for (auto v : src)
        dst  << v.toStdString() << ",";
    return dst;
}

template<> template<>
void object::test<tid_export>()
{
    auto on_exit = setup();
    auto check_clean = cor::on_scope_exit([]() {
            ensure("Root is cleaned", !os::path::exists(root));
        });
    QVariantMap options = {{"dir", vault}, {"bin-dir", vault}
                           , {"home-dir", home}, {"action", "export"}};
    auto args = sys::command_line_options
        (options, short_options, long_options, options_has_param);
    subprocess::check_output("./unit_all", args);
    auto out = str(subprocess::check_output("./check_dirs_similar.sh", QStringList({home, vault})));
    auto lines = out.split("\n").filter(QRegExp("^[<>]"));
    QStringList expected = {"< ./bin/symlink_to_dir", "< ./data/symlink_to_dir"
                            , "> ./" + vault::config::prefix + ".links"
                            , "> ./" + vault::config::prefix + ".unit.version"};
    lines.sort();
    expected.sort();
    ensure_eq("Unexpected structure difference", lines, expected);
}

template<> template<>
void object::test<tid_import>()
{
    auto on_exit = setup();
    QVariantMap options = {
        {"dir", vault}, {"bin-dir", vault}
        , {"home-dir", home_out}, {"action", "import"}};
    auto args = sys::command_line_options
        (options, short_options, long_options, options_has_param);
    subprocess::check_output("./unit_all", args);

    auto out = str(subprocess::check_output
                   ("./check_dirs_similar.sh", QStringList({home, home_out})));
    auto lines = out.split("\n").filter(QRegExp("^[<>]"));
    QStringList expected = {"< ./bin/symlink_to_dir", "< ./data/symlink_to_dir"
                            , "> ./" + vault::config::prefix + ".links"
                            , "> ./" + vault::config::prefix + ".unit.version"};
    lines.sort();
    expected.sort();
    ensure_eq("Unexpected structure difference", lines, expected);
}

}
