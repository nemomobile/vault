#ifndef _VAULT_TESTS_VAULT_CONTEXT_HPP_
#define _VAULT_TESTS_VAULT_CONTEXT_HPP_

#include <subprocess.hpp>
#include <os.hpp>

namespace {


QStringList get_ftree(QString const &root)
{
    Process ps;
    ps.setWorkingDirectory(root);
    auto ftree = filterEmpty(str(ps.check_output("find", {"."})).split("\n"));
    ftree.sort();
    return ftree;
}

QVariantMap context;

void setup_context()
{
    auto home = os::environ("VAULT_TEST_TMP_DIR");
    if (home.isEmpty())
        home = os::mkTemp({{"dir", true}});

    if (home.isEmpty())
        error::raise({{"msg", "Empty home path"}});

    if (!os::path::isDir(home)) {
        if (!os::mkdir(home, {{"parent", true}}))
            error::raise({{"msg", "Can't create tmp dir"}, {"dir", home}});
    }

    auto config_dir = os::environ("VAULT_GLOBAL_CONFIG_DIR");
    if (config_dir.isEmpty())
        config_dir = os::path::join(home, "config");

    context = {
        {"home", home}
        , {"config_dir", config_dir}
        , {"vault_dir", os::path::join(home, ".vault")}
        , {"unit1"
           , map({{"home", map({{"bin", os::path::join("unit1", "binaries")},
                                   {"data", os::path::join("unit1", "data")}})}})}
        , {"unit2"
           , map({{"home", map({{"bin", os::path::join("unit2", "unit2_binaries")},
                                   {"data", os::path::join("unit2", "unit2_data")}})}})}
    };
}

}

#endif // _VAULT_TESTS_VAULT_CONTEXT_HPP_
