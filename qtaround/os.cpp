/**
 * @file os.cpp
 * @brief Wrappers to support API looking familiar for python/shell developers
 * @author Denis Zalevskiy <denis.zalevskiy@jolla.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <qtaround/os.hpp>
#include <qtaround/util.hpp>
#include <qtaround/debug.hpp>

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

QString path::target(QString const &link)
{
    // TODO use c lstat+readlink
    return str(subprocess::check_output("readlink", {link})).split('\n')[0];
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

QByteArray read_file(QString const &fname)
{
    QFile f(fname);
    if (!f.open(QFile::ReadOnly))
        return QByteArray();
    return f.readAll();
}

ssize_t write_file(QString const &fname, QByteArray const &data)
{
    QFile f(fname);
    if (!f.open(QFile::WriteOnly))
        return 0;

    return f.write(data);
}

size_t get_block_size(QString const &cmd_name)
{
    size_t res = 0;
    const QMap<QString, QString> prefices = {{"df", "DF"}, {"du", "DU"}};
    auto prefix = prefices[cmd_name];
    auto names = !prefix.isEmpty() ? QStringList(QStringList({prefix, "BLOCK_SIZE"}).join("_")) : QStringList();
    names += QStringList({"BLOCK_SIZE", "BLOCKSIZE"});
    for (auto it = names.begin(); it != names.end(); ++it) {
        auto const &name = *it;
        auto v = environ(name);
        if (!v.isEmpty()) {
            bool ok = false;
            res = v.toUInt(&ok);
            if (ok)
                break;
        }
    }
    return (res ? res : (is(environ("POSIXLY_CORRECT")) ? 512 : 1024));
}

QList<QVariantMap> mount()
{
    auto line2obj = [](QString const &line) {
        static const QStringList names = {"src", "dst", "type", "options"};
        auto fields = line.split(QRegExp("\\s+"));
        return map(util::zip(names, fields));
    };
    auto split_options = [](QMap<QString, QString> const &fields) {
        QVariantMap res;
        for (auto it = fields.begin(); it != fields.end(); ++it) {
            if (it.key() != "options")
                res.insert(it.key(), it.value());
            else
                res.insert(it.key(), it.value().split(","));
        }
        return res;
    };

    auto data = str(subprocess::check_output("cat", {"/proc/mounts"}));
    auto lines = filterEmpty(data.split("\n"));
    auto fields = util::map<QMap<QString, QString> >(line2obj, lines);
    return util::map<QVariantMap>(split_options, fields);
}

QString mountpoint(QString const &path)
{
    QStringList commands = {"df -P " + path, "tail -1", "awk '{ print $NF; }'"};
    QStringList options = {"-c", commands.join(" | ")};
    auto data = subprocess::check_output("sh", options);
    auto res = str(data).split("\n")[0];
    debug::info("Mountpoint for", path, "=", path);
    return res;
}

/**
 * options.fields - sequence of characters used for field ids used by
 * stat (man 1 stat)
 */
string_map_type stat(QString const &path, QVariantMap &&options)
{
    debug::debug("stat", path, options);

    static const string_map_type long_options = {
        {"filesystem", "file-system"}, {"format", "format"}};

    if (!hasType(options["fields"], QMetaType::QString))
        error::raise({{"msg", "Need to have fields set in options"}});

    auto const &requested = str(options["fields"]);

    auto commify = [](QString const &fields) {
        auto prepend = [](QChar const &c) { return QString("%") + c; };
        return QStringList(util::map<QString>(prepend, fields)).join(",");
    };
    options["format"] = commify(requested);

    auto cmd_options = sys::command_line_options(options, {}, long_options, {"format"});
    cmd_options.push_back(path);

    auto data = str(subprocess::check_output("stat", cmd_options)).trimmed().split(",");
    if (data.size() != requested.size())
        error::raise({{"msg", "Fields set length != stat result length"}
                , {"fields", options["fields"]}
                , {"format", options["format"]}
                , {"result", data}});

    static const string_map_type filesystem_fields = {
        {"b", "blocks"}, {"a", "free_blocks_user"}, {"f", "free_blocks"}
        , {"S", "block_size"}, {"n", "name"}};
    static const string_map_type entry_fields = {
        {"m", "mount_point"}, {"b", "blocks"}, {"B", "block_size"}, {"s", "size"}};


    string_map_type res;
    int i = 0;
    auto const &fields = is(options["filesystem"]) ? filesystem_fields : entry_fields;
    for (auto it = data.begin(); it != data.end(); ++it) {
        auto value = *it;
        auto c = requested[i++];
        auto name = fields[c];
        if (name.isEmpty())
            error::raise({{"msg", "Can't find field name"}, {"id", c}});
        if (c == 'm' && value == "?") {
            // workaround if "m" is not supported
            value = mountpoint(path);
        }
        res[name] = value;
    }
    debug::debug("stat result", path, res);
    return res;
}

class BtrFs {
public:

    BtrFs(QString const &mount_point) : path(mount_point) {}

    QVariantMap df()
    {
        QStringList cmd_options = {"-c", "btrfs fi df " + path};
        auto out = str(subprocess::check_output("sh", cmd_options));
        auto data = filterEmpty(out.trimmed().split("\n"));

        auto split_colon = [](QString const &l) { return l.split(":"); };
        auto name_fields = util::map<QStringList>(split_colon, data);

        auto parse = [](QStringList const &nf) {
            auto parse_size = [](QString const &n_eq_v) {
                auto nv = n_eq_v.trimmed().split("=");
                auto kb = util::parseBytes(nv[1], "kb", kb_bytes);
                return std::make_tuple(nv[0], kb);
            };
            auto names_kbs = util::map<map_tuple_type>(parse_size, nf[1].split(","));
            auto fields = map(names_kbs);
            return std::make_tuple(nf[0], QVariant(fields));
        };
        return map(util::map<map_tuple_type>(parse, name_fields));
    }

    double free()
    {
        debug::debug("btrfs.free for", path);
        auto s = stat(path, {{"fields", "bS"}, {"filesystem", true}});
        auto b = s["blocks"].toLong(), bs = s["block_size"].toLong();
        auto kb = bs / kb_bytes;
        auto total = kb * b;
        auto info = df();

        auto used = util::map<double>([](QString const &, QVariant const &v) {
                auto get_used = [](QString const &k, QVariant const &v) {
                    return (k == "used") ? v.toDouble() : 0.0;
                };
                auto used = util::map<double>(get_used, v.toMap());
                return std::accumulate(used.begin(), used.end(), 0.0);
            }, info);
        return total - std::accumulate(used.begin(), used.end(), 0.0);
    }

private:
    static const size_t kb_bytes = 1024;
    QString path;
};

double diskFree(QString const &path)
{
    debug::debug("diskFree for", path);
    auto s = stat(path, {{"fields", "m"}});
    auto mount_point = s["mount_point"];
    if (mount_point == "?")
        mount_point = mountpoint(path);
    auto mounts = util::mapByField<QString>(mount(), "dst");
    auto info = mounts[mount_point];
    if (info.empty())
        error::raise({{"msg", "Can't find mount point"}
                , {"path", path}
                , {"mounts", QVariant::fromValue(mounts)}});
    double res = 0.0;
    if (str(info["type"]) == "btrfs") {
        res = BtrFs(mount_point).free();
    } else {
        s = stat(mount_point, {{"fields", "aS"}, {"filesystem", true}});
        auto b = s["free_blocks_user"].toDouble(), bs = s["block_size"].toDouble();
        auto kb = bs / 1024;
        res = kb * b;
    }
    debug::debug("diskFree for", path, "=", res);
    return res;
}

QVariant du(QString const &path, QVariantMap &&options)
{
    static const string_map_type short_options = {
        {"summarize", "s"}, {"one_filesystem", "x"},  {"block_size", "B"} };

    if (!options["block_size"].isValid()) // return usage in K
        options["block_size"] = "K";

    auto cmd_options = sys::command_line_options
        (options, short_options, {}, {"block_size"});
    cmd_options.push_back(path);

    auto out = str(subprocess::check_output("du", cmd_options));
    auto data = filterEmpty(out.split("\n"));
    auto extract_size = [](QString const &line) {
        auto s = line.trimmed();
        auto pos = s.indexOf(QRegExp("\\s"));
        if (pos < 0)
            pos = s.size();
        return util::parseBytes(s.left(pos), "K");
    };
    if (data.size() == 1)
        return extract_size(data[0]);

    static const QRegExp spaces_re("\\s");
    auto fields = util::map<QStringList>([](QString const &v) {
            return v.split(spaces_re);
        }, data);
    auto get_sizes = [&extract_size](QStringList const &v) {
        return std::make_tuple(v[1], extract_size(v[0]));
    };
    auto pairs = util::map<map_tuple_type>(get_sizes, fields);
    return map(pairs);
}

QString mkTemp(QVariantMap &&options)
{
    if (options.empty())
        options["dir"] = false;

    string_map_type short_options = {{"dir", "d"}};
    auto args = sys::command_line_options(options, short_options);
    auto res = str(subprocess::check_output("mktemp", args));
    return res.trimmed();
}

} // os
