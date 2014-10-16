#include <vault/unit.hpp>

#include <qtaround/util.hpp>
#include <qtaround/sys.hpp>
#include <qtaround/error.hpp>

#include <QCoreApplication>
#include <QDebug>

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

int main(int argc, char *argv[])
{
    try {
        QCoreApplication app(argc, argv);
        using namespace vault::unit;
        execute(getopt(), info);
    } catch (qtaround::error::Error const &e) {
        qDebug() << e;
    } catch (std::exception const &e) {
        qDebug() << e.what();
    }
    return 0;
}
