#include "unit.hpp"
#include "util.hpp"

namespace {
QVariantMap info = {
    {"home" , map({
                {"data", list({
                            map({{"path", "data/.hidden_dir_self"}})
                                , "data/content/."
                                , map({{"path", "data/file1" }})
                                , map({{"path", "data/in_dir/file2" }})
                                , map({{"path", "data/symlink_to_dir" }})})}
                , {"bin", list({
                            map({{"path", "bin/content/."}})
                                , map({{"path", "bin/.hidden_dir_self" }})
                                , map({{"path", "bin/file1" }})
                                , "bin/in_dir/file2"
                                , "bin/symlink_to_dir"})}})}
};
}

int main()
{
    using namespace vault::unit;
    execute(getopt(), info);
    return 0;
}
