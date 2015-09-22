#ifndef _VAULT_SYNC_HPP_
#define _VAULT_SYNC_HPP_

#include "vault-util.hpp"

#include <cor/util.hpp>

#include <memory>
#include <set>
#include <deque>

enum class Action { Import = 0, Export };

enum class Depth { Shallow = 0, Recursive };
enum class Overwrite { No = 0, Yes };
enum class Deref { No = 0, Yes };
enum class DataHint { Compact = 0, Big };
enum class Update { No = 0, Yes };
enum class Options { Vault, Data, Depth, Overwrite, Deref
        , Update, Last_ = Update };

typedef Record<Options
               , VaultHandle, DataHint, Depth, Overwrite
               , Deref, Update> options_type;

template <> struct RecordTraits<options_type> {
    RECORD_FIELD_NAMES(options_type
                       , "Vault", "Data", "Depth", "Overwrite"
                       , "Deref", "Update");
};

enum class Context { Options, Action, Src, Dst, Last_ = Dst };
typedef Record<Context
               , options_type, Action, QFileInfo, QFileInfo> context_type;
template <> struct RecordTraits<context_type> {
    RECORD_FIELD_NAMES(context_type
                       , "Options", "Action", "Src" , "Dst");
};

class Processor {
public:
    enum class End { Front, Back };
    typedef std::shared_ptr<context_type> data_ptr;
    void add(data_ptr const &, End end = End::Back);
    void execute();
private:

    void onDir(data_ptr const &);
    std::deque<data_ptr> todo_;
    std::set<std::pair<FileId, FileId> > visited_;
};

#endif // _VAULT_SYNC_HPP_
