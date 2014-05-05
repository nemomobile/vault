#include "os.hpp"

namespace os {

QStringList path::split(QString const &p)
{
    auto parts = p.split('/');
    auto len  = parts.size();

    if (len <= 1)
        return parts;

    if (parts[0] == "")
        parts[0] = '/';

    auto end = std::remove_if
        (parts.begin(), parts.end(), [](QString const &v) {
            return v == "";});
    parts.erase(end, parts.end());
    return parts;
}

bool path::isDescendent(QString const &p, QString const &other) {
    auto tested = split(canonical(p));
    auto pivot = split(canonical(other));

    // hardlinks?
    if (pivot.size() > tested.size())
        return false;
    for (int i = 0; i < pivot.size(); ++i) {
        if (pivot[i] != tested[i])
            return false;
    }
    return true;
}

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

int update (QString const &src, QString const &dst, QVariantMap &&options)
{
    options["update"] = true;
    return cp(src, dst, std::move(options));
}

int update_tree(QString const &src, QString const &dst, QVariantMap &&options)
{
    options["deref"] = true;
    options["recursive"] = true;
    return update(src, dst, std::move(options));
}

int cp(QString const &src, QString const &dst, QVariantMap &&options)
{
    string_map_type short_options = {
        {"recursive", "r"}, {"force", "f"}, {"update", "u"}
        , {"deref", "L"}, {"no_deref", "P"},
        {"hardlink", "l"}
        };
    string_map_type long_options = {
        {"preserve", "preserve"}, {"no_preserve", "no-preserve"}
        , {"overwrite", "remove-destination"}
    };

    auto args = sys::command_line_options
        (options, short_options, long_options
         , {{"preserve", "no_preserve"}});

    args += QStringList({src, dst});
    return system("cp", args);
}

int cptree(QString const &src, QString const &dst, QVariantMap &&options)
{
    options["recursive"] = true;
    options["force"] = true;
    return cp(src, dst, std::move(options));
}

}
