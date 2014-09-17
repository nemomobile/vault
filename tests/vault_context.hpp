#ifndef _VAULT_TESTS_VAULT_CONTEXT_HPP_
#define _VAULT_TESTS_VAULT_CONTEXT_HPP_

#include "tests_common.hpp"

#include <qtaround/subprocess.hpp>
#include <qtaround/os.hpp>

#include <tut/tut.hpp>

#include <QSet>

namespace os = qtaround::os;
namespace error = qtaround::error;
namespace util = qtaround::util;

namespace {

using qtaround::subprocess::Process;

QSet<QString> get_ftree(QString const &root)
{
    Process ps;
    ps.setWorkingDirectory(root);
    auto ftree = filterEmpty(str(ps.check_output("find", {"."})).split("\n"));
    ftree.sort();
    return QSet<QString>::fromList(ftree);
}

void ensure_trees_equal(QString const &msg
                        , QSet<QString> const &before
                        , QSet<QString> const &after)
{
    using tut::ensure_eq;
    auto before_wo_after = before - after;
    auto after_wo_before = after - before;
    // compare to see difference on failure
    ensure_eq(S_(msg + ": something is appeared")
              , after_wo_before, QSet<QString>());
    ensure_eq(S_(msg, ": something was removed")
              , before_wo_after, QSet<QString>());
}

QVariant mktree(QVariantMap const &tree, QString const &root)
{
    using tut::ensure;
    auto fn = [](QVariant const &path_v, QVariant const &key, QVariant const &data) {
        auto path = str(path_v);
        if (!key.isValid()) {
            if (!os::path::isDir(path))
                os::mkdir(path);
            return path;
        }
        auto name = str(key);

        auto cur_path = os::path::join(path, name);
        if (hasType(data, QMetaType::QVariantMap)) {
            os::mkdir(cur_path);
            ensure("Dir doesn't exists", os::path::isDir(cur_path));
        } else if (hasType(data, QMetaType::QString)) {
            os::write_file(cur_path, str(data));
            ensure("File is written", os::path::isFile(cur_path));
        }
        return cur_path;
    };
    return util::visit(fn, tree, root);
}

QVariantMap initContext(QString const &home_subdir = "")
{
    QVariantMap context;
    auto home = os::environ("VAULT_TEST_TMP_DIR");
    if (home.isEmpty())
        home = os::mkTemp({{"dir", true}});

    if (home.isEmpty())
        error::raise({{"msg", "Empty tests home path"}});

    if (home == ".." || home == "." || home == os::home())
        error::raise({{"msg", "Can't use directory"}, {"dir", home }});

    if (!(os::path::isDir(home) || os::mkdir(home, {{"parent", true}})))
            error::raise({{"msg", "Can't create tmp dir"}, {"dir", home}});

    if (!home_subdir.isEmpty())
        home = os::path::join(home, home_subdir);

    auto config_dir = os::environ("VAULT_GLOBAL_CONFIG_DIR");
    if (config_dir.isEmpty())
        config_dir = os::path::join(home, "config");

    context = {
        {"home", home}
        , {"config_dir", config_dir}
        , {"vault_dir", os::path::join(home, ".vault")}
        , {"unit1_dir", os::path::join(home, "unit1")}
        , {"unit2_dir", os::path::join(home, "unit2")}
        , {"unit1"
           , map({{"home", map({{"bin", os::path::join("unit1", "binaries")},
                                   {"data", os::path::join("unit1", "data")}})}})}
        , {"unit2"
           , map({{"home", map({{"bin", os::path::join("unit2", "unit2_binaries")},
                                   {"data", os::path::join("unit2", "unit2_data")}})}})}
    };
    return context;
}

QVariantMap context = initContext();

void reinitContext(QString const &home_subdir)
{
    context = initContext(home_subdir);
};

const QVariantMap unit1_tree = {
    {"data", map({{"f1", "data1"}})},
    {"binaries", map({{"b1", "bin data"}, {"b2", "bin data 2"}})}
};

const QVariantMap unit2_tree = {
    {"unit2_data", map({{"f2", "data2"}, {"f3", "data3"}})},
    {"unit2_binaries", map({{"b1", "bin data"}, {"b2", "bin data 2"}})}
};

}

int register_unit(QString const &vault_dir, const QString &name, bool is_global);

#endif // _VAULT_TESTS_VAULT_CONTEXT_HPP_
