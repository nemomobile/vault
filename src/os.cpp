#include "os.hpp"

namespace os {

int system(QString const &cmd, QStringList const &args)
{
    Process p;
    p.start(cmd, args);
    p.wait(-1);
    return p.rc();
}

bool mkdir(QString const &path, QVariantMap const &options)
{
    QVariantMap err = {{"fn", "mkdir"}, {"path", path}};
    if (path::isDir(path))
        return false;
    if (options.value("parent", false).toBool())
        return system("mkdir", {"-p", path}) == 0;
    return system("mkdir", {path}) == 0;
}

}
