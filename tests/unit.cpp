#include "sys.hpp"
#include "os.hpp"

#include <tut/tut.hpp>

//#include "tests_common.hpp"

#include <QDebug>

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
};

namespace {

const string_map_type short_options = {
};

const string_map_type long_options = {
    {"bin-dir", "bin-dir"}, {"data-dir", "data-dir"}
    , {"home-dir", "home"}, {"action", "action"}
};

const QSet<QString> options_has_param = {
    {"bin-dir", "data-dir", "home-dir", "action"}
};

const QString root = "/tmp/test-the-vault-unit";
const QString home = os::path::join(root, "home");
const QString vault = os::path::join(root, "vault");
const QString home_out = os::path::join(root, "home_out");

} // namespace

template<> template<>
void object::test<tid_export>()
{
    QVariantMap options = {{"data-dir", vault}, {"bin-dir", vault}
                           , {"home-dir", home}, {"action", "export"}};
    auto args = sys::command_line_options
        (options, short_options, long_options, options_has_param);
}

}
