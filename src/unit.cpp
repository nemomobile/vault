#include "os.hpp"
#include "util.hpp"
#include "debug.hpp"
#include "json.hpp"
#include "vault_config.hpp"

#include <cor/options.hpp>
#include <QString>

namespace vault { namespace unit {

static const unsigned current_version = 1;
static const QString default_preserve = "mode,ownership,timestamps";

// bool getopt()
// {
// }

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
    typedef cor::OptParse<std::string> option_parser_type;

public:
    Operation(map_type &&o , map_type const &&c)
        : options(std::move(o))
        , context(std::move(c))
        , vault_dir({{"bin", options["bin_dir"]}, {"data", options["data_dir"]}})
        , home(os::path::canonical(str(options["home"])))
    {}

    void execute()
    {
        QString vault_bin_dir = str(options["bin_dir"]),
            vault_data_dir = str(options["data_dir"]),
            home = os::path::canonical(str(options["home"]));
    }

private:

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

    QString get_root_vault_dir(QString const &data_type) {
        auto res = str(vault_dir[data_type]);
        if (res.isEmpty())
            error::raise({{"msg", "Unknown data type is unknown"},
                         {"data_type", data_type}});
        if (!os::path::isDir(res))
            error::raise({{"msg", "Vault dir doesn't exist?"}, {"dir", res }});
        return res;
    }

    map_type options;
    map_type context;
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
        std::function<void(QString const&, QString const&, map_type &&)> fn;
        
        if (!(os::path::isDir(dst) || os::mkdir(dst, {{ "parent", true }}))) {
            error::raise({{"msg", "Can't create destination in vault"}
                    , {"path", dst}});
        }
        
        if (os::path::isDir(src)) {
            fn = os::update_tree;
        } else if (os::path::isFile(src)) {
            fn = os::cp;
        } else {
            error::raise({{"msg", "No handler for this entry type"}
                    , {"path", src}});
        }
        fn(src, dst, std::move(options));
    };

    auto process_symlinks = [this, &links](map_type &v) {
        if (!os::path::isSymLink(str(get(v, "full_path"))))
            return;

        debug::debug("Process symlink", v);
        if (!os::path::isDescendent(str(get(v, "full_path"))
                                    , str(get(v, "root_path")))) {
            if (is(get(v, "required")))
                error::raise({{"msg", "Required path does not belong to its root dir"}
                        , {"path", get(v, "full_path")}});
            v["skip"] = true;
            return;
        }

        // separate link and deref destination
        auto tgt = os::path::deref(str(get(v, "full_path")));
        auto tgt_path = os::path::relative(os::path::canonical(tgt), home);

        map_type link_info;
        insert(link_info, v);
        link_info["target"] = tgt;
        link_info["target_path"] = tgt_path;
        links.add(link_info);

        v["full_path"] = tgt;
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
        
    std::for_each(paths.begin(), paths.end(), process_symlinks);
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

    list_type linked_items;
    
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
        auto src = os::path::join(src_root, str(item["path"]));
        if (os::path::exists(src)) {
            item["src"] = src;
            return map_type();
        }
        auto link = links.get(item);
        item["skip"] = true;

        auto create_link = [](map_type const &link, map_type const &item) {
            create_dst_dirs(item);
            os::symlink(str(link["target_path"]), str(item["full_path"]));
        };
 
        if (link.empty()) {
            if (is(item["required"]))
                error::raise({{"msg", "No required source item"},
                            {"path", src}, {"path", link["path"]}});
            return map_type();
        }

        QVariantMap linked(item);
        linked["path"] = link["target_path"];
        linked["full_path"] = os::path::join(str(item["root_path"])
                                       , str(link["target_path"]));
        src = os::path::join(src_root, str(linked["path"]));
        if (os::path::exists(src)) {
            linked["src"] = src;
            linked["skip"] = false;
            create_link(link, item);
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

    for (auto it = items.begin(); it != items.end(); ++it) {
        auto linked = process_absent_and_links(*it);
        if (!linked.empty())
            linked_items.push_back(std::move(linked));
    }
    items.append(linked_items);

    for (auto it = items.begin(); it != items.end(); ++it) {
        auto &item = *it;
        QString src, dst_dir, dst;
        if (is(item["skip"])) return;
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
                    os::rmtree(dst);
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


}}
