#include <qtaround/debug.hpp>
#include <qtaround/util.hpp>
#include <qtaround/os.hpp>
#include <qtaround/subprocess.hpp>
#include <qtaround/json.hpp>

#include <QCoreApplication>
#include <QDebug>

namespace os = qtaround::os;
namespace error = qtaround::error;
namespace sys = qtaround::sys;
namespace json = qtaround::json;
namespace debug = qtaround::debug;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    try {
        qDebug() << os::path::exists(".")
                 << os::mkdir("./tmp1", {{"parent", true}})
                 << os::mkdir("./tmp2/tmp3", {{"parent", true}})
                 << os::mkdir("tmp4")

            ;
        debug::print(1, map({{"w", 2}}));
        debug::debug("w", "e");
        debug::error("error", "more info", 1);

        auto v = json::read("data.json");
        debug::print(v, get(v, "name"), get(v, "array", 0));
        // is movable
        auto v2 = std::move(v);
        debug::print(v2);

        QVariantMap options_info
            = {{"data_dir", map({{"short", "d"}, {"long", "dir"}
                        , {"required", true}, {"has_param", true}})}
               , {"bin_dir", map({{"short", "b"}, {"long", "bin-dir"}
                        , {"required", true}, {"has_param", true}})}
               , {"home", map({{"short", "H"}, {"long", "home-dir"}
                        , {"required", true}, {"has_param", true}})}
               , {"action", map({{"short", "a"}, {"long", "action"}
                        , {"required", true}, {"has_param", true}})}};

        sys::getopt(options_info);

    } catch(error::Error const &e) {
        qDebug() << "E:" << e.m;
    } catch (std::exception const &e) {
        qDebug() << "E:" << e.what();
    }

    return 0;
}
