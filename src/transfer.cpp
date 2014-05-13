#include "os.hpp"
#include "subprocess.hpp"
#include "debug.hpp"
#include "error.hpp"
#include "vault.hpp"

#include "QString"
#include "QVariant"
#include "QMap"

namespace {

std::shared_ptr<vault::Vault> vault_;
using vault::File;

std::shared_ptr<vault::Vault> getVault()
{
    if (!vault_)
        error::raise({{"msg", "Vault should be initialized first"}});
    return vault_;
}

void invalidateVault()
{
    vault_ = nullptr;
}

}

using debug::Level;

template <typename ... Args>
void trace(Level l, Args &&...args)
{
    debug::print_ge(l, "Vault.transfer:", std::forward<Args>(args)...);
}

enum class Action { Export, Import };

static inline QString str(Action a)
{
    return (a == Action::Export) ? "export" : "import";
}

static QDebug & operator <<(QDebug &d, Action a)
{
    d << str(a);
    return d;
}

struct Archive
{

    Archive(Action a)
        : action(a)
        , space_free(0)
        , space_required(0)
    {}

    Action action;
    QString src;
    QString dst;
    double space_free;
    double space_required;
};

std::unique_ptr<Archive> 
export_import_prepare(Action action, QString const &dump_path)
{
    trace(Level::Debug, "Prepare", action);

    if (!hasType(dump_path, QMetaType::QString))
        error::raise({{"reason", "Logic"}
                , {"message", "Export/import path is bad"}
                , {"path", dump_path}});

    auto path = os::path::join(dump_path, "Backup.tar");
    auto res = cor::make_unique<Archive>(action);
    auto storage = getVault();
    QString dst_dir;
    if (action == Action::Import) {
        if (!os::path::exists(path))
            error::raise({{"reason", "NoSource"}
                    , {"message", "There is nothing to import"}
                    , {"path", path}});
        res->src = path;
        dst_dir = storage->root();
        res->dst = dst_dir;
        if (!os::path::isDir(dst_dir))
            dst_dir = os::path::dirName(dst_dir);
    } else if (action == Action::Export) {
        if (!os::path::exists(storage->root()) || !storage->ensureValid())
            error::raise({{"reason", "NoSource"}, {"message", "Invalid vault"}
                    , {"path", storage->root()}});
        res->src = storage->root();
        res->dst = path;
        dst_dir = os::path::dirName(path);
    } else {
        error::raise({{"reason", "Logic"}, {"message", "Unknown action"}
                , {"action", str(action)}});
    }
    res->space_free = os::diskFree(dst_dir);
    trace(Level::Debug, "dst=", dst_dir, "free space=", res->space_free);
    return res;
}

enum class Io { Exec, Options, OnProgress, EstSize, Dst, EOE };
template <> struct StructTraits<Io>
{
    typedef std::tuple<QString, QStringList
                       , std::function<void(QVariantMap &&)>
                       , long, QString> type;

    STRUCT_NAMES(Io, "Exec", "Options", "OnProgress", "EstSize", "Dst");
};

typedef Struct<Io> IoCmd;

Process do_io(IoCmd const &info)
{
    debug::debug("Io", info);
    Process ps;
    ps.start(info.get<Io::Exec>(), info.get<Io::Options>());
    auto is_finished = false;
    auto dst_size = 0;
    auto dtime = 1000;

    auto calculateDTime = [&info, dst_size](double dtime) {
        auto dst = info.get<Io::Dst>();
        auto dtime_now = dtime;
        auto dst_size_now = os::du(dst, {{"summarize", true}}).toDouble();
        auto dsize = dst_size_now - dst_size;
        info.get<Io::OnProgress>()({{"type", "dst_size"}
                , {"size", dst_size_now}});

        if (info.get<Io::EstSize>() > 0 && dsize > 0) {
            auto size_fraction = dsize / info.get<Io::EstSize>();
            if (size_fraction > 0.2) {
                dtime_now = 1000;
            } else if (size_fraction < 0.02) {
                dtime_now = 5000;
            } else {
                dtime_now = 2000;
            }
            if (dtime_now != dtime)
                trace(Level::Debug, "New dT=", dtime);
        }

        return std::make_tuple(dtime_now, dst_size_now);
    };

    if (!info.get<Io::OnProgress>()) {
        ps.wait(-1);
        return std::move(ps);
    }

    is_finished = ps.wait(dtime);
    while (!is_finished) {
        std::tie(dtime, dst_size) = calculateDTime(dtime);
        is_finished = ps.wait(dtime);
    }
    return std::move(ps);
}

std::unique_ptr<Archive> estimate_space(std::unique_ptr<Archive> msg)
{
    if (!msg->space_required) {
        trace(Level::Debug, "Check required space");
        auto is_src_dir = os::path::isDir(msg->src);
        // if source is directory - it is .vault, so only .git is packed
        auto real_src = is_src_dir ? os::path::join(msg->src, ".git") : msg->src;
        msg->space_required = get<double>
            (os::du(real_src, {{"summarize", is_src_dir}}));
        trace(Level::Debug, "du=", msg->space_required);
        if (!is_src_dir) {
            // empiric multiplier: unpacked
            // files can take more space,
            // it depends on fs
            msg->space_required *= 1.2;
        }
        if (os::path::exists(msg->dst)) {
            auto to_be_freed = get<double>
                (os::du(msg->dst, {{"summarize", !is_src_dir}}));
            msg->space_free += to_be_freed;
        }
        trace(Level::Debug, "total required=", msg->space_required
              , "free=", msg->space_free);
    }
    if (msg->space_required > msg->space_free)
        error::raise({{"reason", "NoSpace"}, {"free", msg->space_free}
                     , {"required", msg->space_required}});
    return msg;
}

void validate_storage_dump(QString const &archive, QVariantMap const &err)
{
    trace(Level::Debug, "Validate dump", archive);
    QString prefix(".git/");
    auto prefix_len = prefix.size();
    auto tag_fname = vault::fileName(File::State);
    auto tag_len = tag_fname.size();
    auto is_full_dump = false;

    auto out = str(subprocess::check_output
                   ("tar", {{"-tf", archive}}, {{"reason", "Archive"}}));
    auto lines = out.split("\n").filter(QRegExp("^.+$"));
    for (auto it = lines.begin(); it != lines.end(); ++it) {
        auto const &fname = *it;
        if (fname.left(prefix_len) != prefix) {
            if (fname.left(tag_len) != tag_fname) {
                error::raise(err, map({{"msg", "Unexpected file name"}
                            , {"fname", fname}, {"dump", archive}}));
            }
            is_full_dump = true;
        }
    }
    if (!is_full_dump)
        error::raise(err, map({{"message", "No tag file found"}
                    , {"dump", archive}}));
}

void export_storage(std::unique_ptr<Archive> msg
                    , std::function<void(QVariantMap&&)> reply)
{
    auto tag_fname = vault::fileName(File::State);
    QStringList options = {"-cf", msg->dst, "-C", msg->src, ".git", tag_fname};
    reply({{"type", "stage"}, {"stage", "Copy"}});
    try {
        IoCmd cmd("tar", options, reply, msg->space_required, msg->dst);
        auto ps = do_io(cmd);
        ps.check_error({{"reason", "Export"}});

        reply({{"type", "stage"}, {"stage", "Flush"}});
        subprocess::check_call("sync", {}, {{"reason", "Export"}});

        reply({{"type", "stage"}, {"stage", "Validate"}});
        validate_storage_dump(msg->dst, {{"reason", "Export"}});

    } catch (...) {
        if (os::path::exists(msg->dst))
            os::rm(msg->dst);
        throw;
    }
}

void import_storage(std::unique_ptr<Archive> msg
                    , std::function<void(QVariantMap&&)> reply)
{
    trace(Level::Debug, reply);
    auto root = getVault()->root();
    if (msg->dst != root)
        error::raise({{"reason", "Logic"}, {"message", "Invalid destination"}
                     , {"path", msg->dst}});

    if (os::path::exists(root) && !getVault()->ensureValid())
        error::raise({{"reason", "Logic"}, {"message", "Invalid vault"}
                , {"path", root}});

    reply({{"type", "stage"}, {"stage", "Validate"}});
    validate_storage_dump(msg->src, {{"reason", "BadSource"}});
    trace(Level::Debug, "Clean destination tree");
    os::rmtree(msg->dst);
    invalidateVault();
    if (os::path::exists(msg->dst))
        error::raise({{"reason", "unknown"}, {"message", "Can't remove destination"}
                , {"dst", msg->dst}});

    try {
        os::mkdir(msg->dst);
        QStringList options = {"-xpf", msg->src, "-C", msg->dst};
        reply({{"type", "stage"}, {"stage", "Copy"}});
        IoCmd cmd("tar", options, reply, msg->space_required, msg->dst);
        auto res = do_io(cmd);
        res.check_error({{"reason", "Archive"}});
    } catch(...) {
        if (os::path::exists(msg->dst))
            os::rmtree(msg->dst);
        throw;
    }
}

void export_import(std::unique_ptr<Archive> msg
                   , std::function<void(QVariantMap&&)> reply)
{
    trace(Level::Debug, "Export/import", "msg:", msg);

    if (!os::path::exists(msg->src))
        error::raise({{"reason", "Logic"}
                , {"message", "Source does not exist"}
                , {"src", msg->src}});

    msg = estimate_space(std::move(msg));
    reply({{"type", "estimated_size"}, {"size", msg->space_required}});

    switch (msg->action) {
    case Action::Export:
        export_storage(std::move(msg), reply);
        break;
    case Action::Import:
        import_storage(std::move(msg), reply);
        break;
    default:
        error::raise({{"reason", "Logic"}, {"message", "Unknown action"}
                , {"action", str(msg->action)}});
    }
}
