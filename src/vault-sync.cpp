#include "vault-sync.hpp"
#include "file-util.hpp"
#include "vault-util.hpp"
#include <qtaround/error.hpp>
#include <qtaround/os.hpp>
#include <QCoreApplication>

namespace error = qtaround::error;
namespace os = qtaround::os;

QDebug & operator << (QDebug &dst, QFileInfo const &v)
{
    dst << str(v);
    return dst;
}

static FileHandle copy_blob(Action action
                            , QString const &dst_path
                            , QFileInfo const &from
                            , VaultHandle const &root)
{
    if (action == Action::Export) {
        auto data_hash = root->hash_file(from.filePath());
        debug::debug("Hash: ", data_hash, " for ", from.filePath());
        auto data_dst_path = root->blob_path(data_hash);
        QFileInfo data_stat(data_dst_path);
        if (!data_stat.exists()) {
            auto blob_dir = os::path::dirName(data_stat);
            mkdir(blob_dir, permissions::dirForUserOnly);
            copy_data(data_dst_path, from, nullptr);
        }
        return rewrite(dst_path, root->uri_from_hash(data_hash)
                       , from.permissions());
    } else {
        auto data_uri = read_text(from.filePath(), VAULT_URI_MAX_SIZE).trimmed();
        QFileInfo data_stat(root->path_from_uri(data_uri));
        auto mode = from.permissions();
        return copy_data(dst_path, data_stat, &mode);
    }
}

static QFileInfo file_copy(QFileInfo const &from, QFileInfo const &parent
                           , options_type const &options
                           , Action action)
{
    debug::debug("Copy file", from, parent);
    auto dst_path = path(parent.filePath(), from.fileName());
    QFileInfo dst_stat(dst_path);
    if (dst_stat.exists()) {
        if (options.get<Options::Overwrite>() == Overwrite::No) {
            debug::debug("Do not overwrite", dst_stat.filePath());
            if (options.get<Options::Update>() == Update::No
                || !is_older(dst_stat.filePath(), from.filePath()))
                return std::move(dst_stat);
        }
        if (!dst_stat.isFile()) {
            if (dst_stat.isSymLink())
                unlink(dst_stat.filePath());
            else
                return std::move(dst_stat);
        }
    }
    auto mode = from.permissions();
    auto dst = (options.get<Options::Data>() == DataHint::Big
                ? copy_blob(action, dst_path, from
                            , options.get<Options::Vault>())
                : copy_data(dst_path, from, &mode));
    copy_utime(*dst, from);
    dst->close();
    dst_stat.refresh();
    return std::move(dst_stat);
}

void Processor::add(data_ptr const &p, End end)
{
    debug::debug("Adding", *p);
    auto push = [this, end](data_ptr const &p) {
        if (end == End::Back)
            todo_.push_back(p);
        else
            todo_.push_front(p);
    };
    auto const &opts = p->get<Context::Options>();
    auto const &src_stat = p->get<Context::Src>();
    if (src_stat.isDir()) {
        if (opts.get<Options::Depth>() == Depth::Recursive)
            push(p);
        else
            debug::info("Omiting directory", src_stat.filePath());
    } else {
        push(p);
    }
}

void Processor::execute()
{
    auto onFile = [this](context_type const &ctx) {
        auto const &src = ctx.get<Context::Src>();
        auto const &tgt = ctx.get<Context::Dst>();
        debug::debug("File: ", src);
        auto dst = file_copy(src, tgt
                             , ctx.get<Context::Options>()
                             , ctx.get<Context::Action>());
    };
    auto onSymlink = [this](context_type const &ctx) {
        auto const &src = ctx.get<Context::Src>();
        debug::debug("Symlink: ", src);
        auto const &dst = ctx.get<Context::Dst>();
        auto const &opts = ctx.get<Context::Options>();
        if (opts.get<Options::Deref>() == Deref::Yes) {
            debug::debug("Deref", str(src));
            auto new_path = readlink(src.filePath());
            auto new_ctx = std::make_shared<context_type>(ctx);
            new_ctx->get<Context::Src>() = QFileInfo(new_path);
            this->add(new_ctx, End::Front);
        } else {
            QFileInfo dst_link(path(dst, src.fileName()));
            if (exists(dst_link)) {
                if (opts.get<Options::Overwrite>() != Overwrite::Yes) {
                    debug::debug("Skip existing symlink", str(dst_link));
                    return;
                }
                unlink(dst_link.filePath());
            }
            symlink(readlink(src).filePath(), dst_link.filePath());
        }
    };
    auto onDir = [this](context_type const &ctx) {
        auto const &options = ctx.get<Context::Options>();
        auto const &src = ctx.get<Context::Src>();
        debug::debug("Dir: ", src);
        auto const &target = ctx.get<Context::Dst>();
        auto is_overwrite = (options.get<Options::Overwrite>() == Overwrite::Yes);
        auto copy_time = is_overwrite ? CopyTime::Always : CopyTime::NewOnly;
        auto dst = mkdir_similar(src, target, copy_time);

        auto entries = QDir(src.filePath()).entryInfoList
        (QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Hidden);
        debug::debug("There are", entries.size(), " entries in the", src);
        while (!entries.isEmpty()) {
            auto entry = entries.takeFirst();

            debug::debug("Entry", entry.fileName());
            auto item = std::make_shared<context_type>
                (options, ctx.get<Context::Action>()
                 , QFileInfo(entry.filePath()), dst);
            this->add(item, End::Front);
        }
    };
    auto operationId = [](context_type const &ctx) {
        auto op_id = std::make_pair(fileId(ctx.get<Context::Src>())
                                    , fileId(ctx.get<Context::Dst>()));
        debug::debug("Operation", op_id, " for ", ctx);
        return std::move(op_id);
    };
    auto markVisited = [operationId, this](context_type const &ctx) {
        visited_.insert(operationId(ctx));
    };
    auto isVisited = [operationId, this](context_type const &ctx) {
        return (visited_.find(operationId(ctx)) != visited_.end());
    };

    while (!todo_.empty()) {
        auto item = todo_.front();
        todo_.pop_front();
        auto const &src = item->get<Context::Src>();
        auto &dst = item->get<Context::Dst>();
        debug::debug("Processing", src.filePath());
        dst.refresh();
        if (!isVisited(*item)) {
            if (src.isSymLink()) {
                onSymlink(*item);
            } else if (src.isDir()) {
                onDir(*item);
            } else if (src.isFile()) {
                onFile(*item);
            } else {
                debug::debug("No handler for the type", src.filePath());
                break;
            }
            markVisited(*item);
        } else {
            debug::info("Skip duplicate", *item);
        }
    }
}
