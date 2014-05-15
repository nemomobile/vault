#include "transfer.hpp"

#include "os.hpp"
#include "subprocess.hpp"
#include "debug.hpp"
#include "error.hpp"
#include <vault/vault.hpp>

#include "QString"
#include "QVariant"
#include "QMap"

namespace {

using vault::File;


}

using debug::Level;

template <typename ... Args>
void trace(Level l, Args &&...args)
{
    debug::print_ge(l, "Vault.transfer:", std::forward<Args>(args)...);
}

static inline QString str(CardTransfer::Action a)
{
    std::array<char const *, (size_t)CardTransfer::ActionsEnd> names
        = {{"export", "import"}};
    return names.at(static_cast<size_t>(a));
}

static QDebug & operator <<(QDebug &d, CardTransfer::Action a)
{
    d << str(a);
    return d;
}

vault::Vault *CardTransfer::getVault()
{
    if (!vault_)
        error::raise({{"msg", "Vault should be initialized first"}});
    return vault_;
}

void CardTransfer::invalidateVault()
{
    vault_ = nullptr;
    emit vaultChanged();
}

void CardTransfer::init(vault::Vault *v, Action action, QString const &dump_path)
{
    trace(Level::Debug, "Prepare", action);
    vault_ = v;

    if (!hasType(dump_path, QMetaType::QString))
        error::raise({{"reason", "Logic"}
                , {"message", "Export/import path is bad"}
                , {"path", dump_path}});

    auto path = os::path::join(dump_path, "Backup.tar");
    auto storage = getVault();
    QString dst_dir;
    if (action == Action::Import) {
        if (!os::path::exists(path))
            error::raise({{"reason", "NoSource"}
                    , {"message", "There is nothing to import"}
                    , {"path", path}});
        src_ = path;
        dst_dir = storage->root();
        dst_ = dst_dir;
        if (!os::path::isDir(dst_dir))
            dst_dir = os::path::dirName(dst_dir);
    } else if (action == Action::Export) {
        if (!os::path::exists(storage->root()) || !storage->ensureValid())
            error::raise({{"reason", "NoSource"}, {"message", "Invalid vault"}
                    , {"path", storage->root()}});
        src_ = storage->root();
        dst_ = path;
        dst_dir = os::path::dirName(path);
    } else {
        error::raise({{"reason", "Logic"}, {"message", "Unknown action"}
                , {"action", str(action)}});
    }
    space_free_ = os::diskFree(dst_dir);
    action_ = action;
    trace(Level::Debug, "dst=", dst_dir, "free space=", space_free_);
}


Process CardTransfer::doIO(IoCmd const &info)
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

void CardTransfer::estimateSpace()
{
    if (!space_required_) {
        trace(Level::Debug, "Check required space");
        auto is_src_dir = os::path::isDir(src_);
        // if source is directory - it is .vault, so only .git is packed
        auto real_src = is_src_dir ? os::path::join(src_, ".git") : src_;
        space_required_ = get<double>(os::du(real_src, {{"summarize", is_src_dir}}));
        trace(Level::Debug, "du=", space_required_);
        if (!is_src_dir) {
            // empiric multiplier: unpacked
            // files can take more space,
            // it depends on fs
            space_required_ *= 1.2;
        }
        if (os::path::exists(dst_)) {
            auto to_be_freed = get<double>
                (os::du(dst_, {{"summarize", !is_src_dir}}));
            space_free_ += to_be_freed;
        }
        trace(Level::Debug, "total required=", space_required_
              , "free=", space_free_);
    }
    if (space_required_ > space_free_)
        error::raise({{"reason", "NoSpace"}, {"free", space_free_}
                     , {"required", space_required_}});
}

void CardTransfer::validateDump(QString const &archive, QVariantMap const &err)
{
    trace(Level::Debug, "Validate dump", archive);
    QString prefix(".git/");
    auto prefix_len = prefix.size();
    auto tag_fname = vault::fileName(File::State);
    auto tag_len = tag_fname.size();
    auto is_full_dump = false;

    auto out = str(subprocess::check_output
                   ("tar", {{"-tf", archive}}, {{"reason", "Archive"}}));
    auto lines = filterEmpty(out.split("\n"));
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

void CardTransfer::exportStorage(CardTransfer::progressCallback onProgress)
{
    auto tag_fname = vault::fileName(File::State);
    QStringList options = {"-cf", dst_, "-C", src_, ".git", tag_fname};
    onProgress({{"type", "stage"}, {"stage", "Copy"}});
    try {
        IoCmd cmd("tar", options, onProgress, space_required_, dst_);
        auto ps = doIO(cmd);
        ps.check_error({{"reason", "Export"}});

        onProgress({{"type", "stage"}, {"stage", "Flush"}});
        subprocess::check_call("sync", {}, {{"reason", "Export"}});

        onProgress({{"type", "stage"}, {"stage", "Validate"}});
        validateDump(dst_, {{"reason", "Export"}});

    } catch (...) {
        if (os::path::exists(dst_))
            os::rm(dst_);
        throw;
    }
}

void CardTransfer::importStorage(CardTransfer::progressCallback onProgress)
{
    trace(Level::Debug, onProgress);
    auto root = getVault()->root();
    if (dst_ != root)
        error::raise({{"reason", "Logic"}, {"message", "Invalid destination"}
                     , {"path", dst_}});

    if (os::path::exists(root) && !getVault()->ensureValid())
        error::raise({{"reason", "Logic"}, {"message", "Invalid vault"}
                , {"path", root}});

    onProgress({{"type", "stage"}, {"stage", "Validate"}});
    validateDump(src_, {{"reason", "BadSource"}});
    trace(Level::Debug, "Clean destination tree");
    os::rmtree(dst_);
    invalidateVault();
    if (os::path::exists(dst_))
        error::raise({{"reason", "unknown"}, {"message", "Can't remove destination"}
                , {"dst", dst_}});

    try {
        os::mkdir(dst_);
        QStringList options = {"-xpf", src_, "-C", dst_};
        onProgress({{"type", "stage"}, {"stage", "Copy"}});
        IoCmd cmd("tar", options, onProgress, space_required_, dst_);
        auto res = doIO(cmd);
        res.check_error({{"reason", "Archive"}});
    } catch(...) {
        if (os::path::exists(dst_))
            os::rmtree(dst_);
        throw;
    }
}

void CardTransfer::execute(CardTransfer::progressCallback onProgress)
{
    // TODO trace(Level::Debug, "Export/import", "msg:", msg);

    if (!os::path::exists(src_))
        error::raise({{"reason", "Logic"}
                , {"message", "Source does not exist"}
                , {"src", src_}});

    estimateSpace();
    onProgress({{"type", "estimated_size"}, {"size", space_required_}});

    switch (action_) {
    case Action::Export:
        exportStorage(onProgress);
        break;
    case Action::Import:
        importStorage(onProgress);
        break;
    default:
        error::raise({{"reason", "Logic"}, {"message", "Unknown action"}
                , {"action", str(action_)}});
        break;
    }
}
