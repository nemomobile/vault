#include "debug.hpp"
#include "util.hpp"
#include "os.hpp"
#include "subprocess.hpp"
#include "json.hpp"
#include <QCoreApplication>
#include <QDebug>

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
        debug::print(v);
    } catch(error::Error const &e) {
        qDebug() << "E:" << e.m;
    } catch (std::exception const &e) {
        qDebug() << "E:" << e.what();
    }
    
    return 0;
}
