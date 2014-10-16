/**
 * @file unit.hpp
 * @brief Vault unit option parsing and file export/import support
 * @author Denis Zalevskiy <denis.zalevskiy@jolla.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <vault/unit.hpp>

#include <qtaround/util.hpp>
#include <qtaround/os.hpp>
#include <qtaround/json.hpp>
#include <qtaround/debug.hpp>

#include <QString>

namespace os = qtaround::os;
namespace error = qtaround::error;
namespace sys = qtaround::sys;
namespace util = qtaround::util;
namespace json = qtaround::json;
namespace debug = qtaround::debug;

namespace vault { namespace unit {

static const unsigned current_version = 1;
static const QString default_preserve = "mode,ownership,timestamps";

namespace {
QVariantMap options_info
= {{"dir", map({{"short", "d"}, {"long", "dir"}
                , {"required", true}, {"has_param", true}})}
   , {"bin-dir", map({{"short", "b"}, {"long", "bin-dir"}
                , {"required", true}, {"has_param", true}})}
   , {"home-dir", map({{"short", "H"}, {"long", "home-dir"}
                , {"required", true}, {"has_param", true}})}
   , {"action", map({{"short", "a"}, {"long", "action"}
                , {"required", true}, {"has_param", true}})}};

class Config
{
public:
    void add(QString const &name, QString const &script);
    void remove(QString const &name);
};


typedef QVariantMap map_type;
typedef QList<QVariantMap> list_type;

class Version {
public:
    Version(QString const &root)
        : fname(os::path::join(root, vault::config::prefix + ".unit.version"))
    {}
    unsigned get()
    {
        return os::path::exists(fname)
            ? QString::fromUtf8(os::read_file(fname)).toInt()
            : 0;
    }
    void save()
    {
        os::write_file(fname, "1");
    }
private:
    QString fname;
};

class Operation
{
public:
    Operation(std::unique_ptr<sys::GetOpt> o, map_type const &c)
        : options(std::move(o))
        , context(c)
        , vault_dir({{"bin", options->value("bin-dir")}, {"data", options->value("dir")}})
        , home(os::path::canonical(options->value("home-dir")))
    {
    }

    void execute();
private:

    typedef std::function<void (QString const &, std::unique_ptr<list_type>
                                , map_type const &)> action_type;

    class Links
    {
    public:
        Links(map_type &&d, QString const &root)
            : data(std::move(d))
            , root_dir(root)
        {}

        void add(map_type const &info) {
            map_type value = {{ "target", info["target"]}
                              , {"target_path", info["target_path"]}};
            data.insert(str(info["path"]), std::move(value));
        }

        void save() {
            if (!data.empty())
                write_links(data, root_dir);
        }

        map_type get(map_type const &info) {
            return data[str(info["path"])].toMap();
        }

        map_type data;
        QString root_dir;
    };

    void to_vault(QString const &data_type
                  , std::unique_ptr<list_type> paths_ptr
                  , map_type const &location);

    void from_vault(QString const &data_type
                    , std::unique_ptr<list_type> paths_ptr
                    , map_type const &location);

    static map_type read_links(QString const &root_dir) {
        return json::read(get_link_info_fname(root_dir)).toVariantMap();
    }

    static size_t write_links(map_type const &links, QString const &root_dir)
    {
        return json::write(links, get_link_info_fname(root_dir));
    }

    static QString get_link_info_fname(QString const &root)
    {
        return os::path::join(root, vault::config::prefix + ".links");
    }

    Version version(QString const &root)
    {
        return Version(root);
    }

    QString get_root_vault_dir(QString const &data_type);

    std::unique_ptr<sys::GetOpt> options;
    map_type const &context;
    map_type vault_dir;
    QString home;
};

void create_dst_dirs(map_type const &item)
{
    auto path = str(item["full_path"]);
    if (!os::path::isDir(str(item["src"])))
        path = os::path::dirName(path);

    if (!os::path::isDir(path)) {
        if (!os::mkdir(path, {{"parent", true}}) && is(item["required"]))
            error::raise({{"msg", "Can't recreate tree to required item"},
                        {"path", item["path"]},
                            {"dst_dir", path}});
    }
}

QString Operation::get_root_vault_dir(QString const &data_type)
{
    auto res = str(vault_dir[data_type]);
    if (res.isEmpty())
        error::raise({{"msg", "Unknown data type is unknown"},
                    {"data_type", data_type}});
    if (!os::path::isDir(res))
        error::raise({{"msg", "Vault dir doesn't exist?"}, {"dir", res }});
    return res;
}

void Operation::to_vault(QString const &data_type
                         , std::unique_ptr<list_type> paths_ptr
                         , map_type const &location)
{
    auto paths = unbox(std::move(paths_ptr));
    debug::debug("To vault", data_type, "Paths", paths, "Location", location);
    auto dst_root = get_root_vault_dir(data_type);
    auto link_info_path = get_link_info_fname(dst_root);
    Links links(read_links(dst_root), dst_root);

    auto copy_entry = [dst_root](map_type const &info) {
        debug::debug("COPY", info);
        auto dst = os::path::dirName(os::path::join(dst_root, str(info["path"])));
        auto src = str(info["full_path"]);
        map_type options = {{"preserve", default_preserve}};
        int (*fn)(QString const&, QString const&, map_type &&);

        if (!(os::path::isDir(dst) || os::mkdir(dst, {{ "parent", true }}))) {
            error::raise({{"msg", "Can't create destination in vault"}
                    , {"path", dst}});
        }

        if (os::path::isDir(src)) {
            fn = &os::update_tree;
        } else if (os::path::isFile(src)) {
            fn = &os::cp;
        } else {
            error::raise({{"msg", "No handler for this entry type"}, {"path", src}});
        }
        fn(src, dst, std::move(options));
    };

    auto process_symlink = [this, &links](map_type &v) {
        auto full_path = str(get(v, "full_path"));

        if (!os::path::isSymLink(full_path))
            return;

        debug::debug("Process symlink", v);
        if (!os::path::isDescendent(full_path, str(get(v, "root_path")))) {
            if (is(get(v, "required")))
                error::raise({{"msg", "Required path does not belong to its root dir"}
                        , {"path", full_path}});
            v["skip"] = true;
            return;
        }

        // separate link and deref destination
        auto tgt = os::path::target(full_path);
        full_path = os::path::canonical(os::path::deref(full_path));
        auto tgt_path = os::path::relative(full_path, home);

        map_type link_info(v);
        link_info.unite(v);
        link_info["target"] = tgt;
        link_info["target_path"] = tgt_path;
        debug::debug("Symlink info", link_info);
        links.add(link_info);

        v["full_path"] = full_path;
        v["path"] = tgt_path;
    };

    auto is_src_exists = [](map_type const &info) {
        auto res = true;
        auto full_path = str(info["full_path"]);
        if (is(info["skip"])) {
            res = false;
        } else if (!os::path::exists(full_path)) {
            if (is(info["required"]))
                error::raise({{"msg", "Required path does not exist"}
                        , {"path", full_path}});
            res = false;
        }
        if (!res)
            debug::info("Does not exist/skip", info);

        return res;
    };

    std::for_each(paths.begin(), paths.end(), process_symlink);
    list_type existing_paths;
    for (auto it = paths.begin(); it != paths.end(); ++it) {
        if (is_src_exists(*it))
            existing_paths.push_back(*it);
    }
    std::for_each(existing_paths.begin(), existing_paths.end(), copy_entry);
    links.save();
    version(dst_root).save();
}

void Operation::from_vault(QString const &data_type
                           , std::unique_ptr<list_type> paths_ptr
                           , map_type const &location)
{
    auto items = unbox(std::move(paths_ptr));
    debug::debug("From vault", data_type, "Paths", items, "Location", location);
    QString src_root(get_root_vault_dir(data_type));

    bool overwrite_default;
    {
        auto v = get(location, "options", "overwrite");
        if (!v.isValid())
            v = get(context, "options", "overwrite");
        overwrite_default = is(v);
    }

    Links links(read_links(src_root), src_root);

    auto fallback_v0 = [src_root, &items]() {
        debug::warning("Restoring from old unit version");
        if (items.empty())
            error::raise({{"msg", "There should be at least 1 item"}});
        // during migration from initial format old (and single)
        // item is going first
        auto dst = os::path::join(str(items[0]["full_path"]));
        if (!os::path::isDir(dst)) {
            if (!os::mkdir(dst, {{"parent", true}}))
                error::raise({{"msg", "Can't create directory"}, {"dir", dst}});
        }
        os::update_tree(os::path::join(src_root, "."), dst
                        , {{"preserve", default_preserve}, {"deref", true}});
    };

    auto process_absent_and_links = [src_root, &links](map_type &item) {
        auto item_path = str(item["path"]);
        auto src = os::path::join(src_root, item_path);
        if (os::path::exists(src)) {
            item["src"] = src;
            return map_type();
        }
        auto link = links.get(item);
        item["skip"] = true;

        auto create_link = [](map_type const &link, map_type const &item) {
            create_dst_dirs(item);
            os::symlink(str(link["target"]), str(item["full_path"]));
        };

        if (link.empty()) {
            debug::debug("No symlink for", item_path);
            if (is(item["required"]))
                error::raise({{"msg", "No required source item"},
                            {"path", src}, {"path", link["path"]}});
            return map_type();
        }

        debug::debug("There is a symlink for", item_path);
        QVariantMap linked(item);
        auto target_path = str(link["target_path"]);
        linked["path"] = target_path;
        linked["full_path"] = os::path::join(str(item["root_path"]), target_path);
        src = os::path::join(src_root, target_path);
        if (os::path::exists(src)) {
            linked["src"] = src;
            linked["skip"] = false;
            create_link(link, item);
            debug::debug("Symlink target path is", linked);
            return linked;
        } else if (is(item["required"])) {
            error::raise({{"msg", "No linked source item"},
                        {"path", src}, {"link", link["path"]}
                        , {"target", linked["path"]}});
        }
        return map_type();
    };

    auto v = version(src_root).get();
    if (v > current_version) {
        error::raise({{"msg", "Can't restore from newer unit version"
                        ", upgrade vault"},
                    {"expected", current_version}, {"actual", v}});
    } else if (v < current_version) {
        return fallback_v0();
    }

    list_type linked_items;
    for (auto it = items.begin(); it != items.end(); ++it) {
        auto linked = process_absent_and_links(*it);
        if (!linked.empty())
            linked_items.push_back(std::move(linked));
    }
    items.append(linked_items);
    debug::debug("LINKED+", items);

    for (auto it = items.begin(); it != items.end(); ++it) {
        auto &item = *it;
        QString src, dst_dir, dst;
        if (is(item["skip"])) {
            debug::debug("Skipping", str(item["path"]));
            continue;
        }
        bool overwrite;
        std::function<void()> fn;

        map_type options = {{"preserve", default_preserve}, {"deref", true}};
        {
            auto v = item["overwrite"];
            overwrite = v.isValid() ? is(v) : overwrite_default;
        }

        // TODO process correctly self dir (copy with dir itself)
        create_dst_dirs(item);
        src = str(item["src"]);
        if (os::path::isDir(src)) {
            dst = os::path::canonical(str(item["full_path"]));
            dst_dir = os::path::dirName(dst);
            src = os::path::canonical(src);
            if (overwrite) {
                options["force"] = true;
                fn = [src, dst, dst_dir, &options]() {
                    os::cptree(src, dst_dir, std::move(options));
                };
            } else {
                fn = [src, dst, dst_dir, &options]() {
                    os::update_tree(src, dst_dir, std::move(options));
                };
            }
        } else if (os::path::isFile(src)) {
            dst = str(item["full_path"]);
            dst_dir = os::path::dirName(dst);

            if (overwrite) {
                options["force"] = true;
                fn = [src, dst, dst_dir, &options]() {
                    os::unlink(dst);
                    os::cp(src, dst_dir, std::move(options));
                };
            } else {
                fn = [src, dst, &options]() {
                    os::cp(src, dst, std::move(options));
                };
            }
        }
        fn();
    }
}

void Operation::execute()
{
    debug::info("Unit execute. Context:", context);
    QString vault_bin_dir = str(options->value("bin-dir")),
        vault_data_dir = str(options->value("dir"));

    action_type action;

    if (!os::path::isDir(home))
        error::raise({{"msg", "Home dir doesn't exist"}, {"dir", home}});

    auto action_name = options->value("action");
    using namespace std::placeholders;
    if (action_name == "export") {
        action = std::bind(&Operation::to_vault, this, _1, _2, _3);
    } else if (action_name == "import") {
        action = std::bind(&Operation::from_vault, this, _1, _2, _3);
    } else {
        error::raise({{ "msg", "Unknown action"}, {"action", options->value("action")}});
    }

    auto get_home_path = [this](QVariant const &item) {
        map_type res;
        if (hasType(item, QMetaType::QString)) {
            res["path"] = str(item);
        } else if (hasType(item, QMetaType::QVariantMap)) {
            res.unite(item.toMap());
        } else {
            error::raise({{"msg", "Unexpected path type"}, {"item", item}});
        }

        auto path = str(res["path"]);
        if (path.isEmpty())
            error::raise({{"msg", "Invalid data(path)"}, {"item", item}});

        res["full_path"] = os::path::join(home, path);
        res["root_path"] = home;
        return res;
    };

    auto process_home_path = [&action, get_home_path](map_type const &location) {
        for (auto it = location.begin(); it != location.end(); ++it) {
            auto const &name = it.key();
            auto const &items = it.value();
            if (name == "options")
                continue; // skip options
            auto data_type = name;
            auto paths = (hasType(items, QMetaType::QString)
                          ? list_type({get_home_path(items)})
                          : util::map<map_type>(get_home_path, items.toList()));
            action(data_type, box(std::move(paths)), location);
        };
    };

    for (auto it = context.begin(); it != context.end(); ++it) {
        auto const &name = it.key();
        auto const &value = it.value();
        if (name == "options")
            continue; // skip options

        if (name == "home") {
            process_home_path(map(value));
        } else {
            error::raise({{"msg", "Unknown context item"}, {"item", name}});
        }
    };

}

} // namespace

std::unique_ptr<sys::GetOpt> getopt()
{
    return sys::getopt(options_info);
}

int execute(std::unique_ptr<sys::GetOpt>, QVariantMap const &info)
{
    Operation op(getopt(), info);
    op.execute();
    // TODO catch and return
    return 0;
}

}} // namespace vault::unit
