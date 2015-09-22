#ifndef _VAULT_TESTS_VAULT_CONTEXT_HPP_
#define _VAULT_TESTS_VAULT_CONTEXT_HPP_

#include "tests_common.hpp"

#include <qtaround/os.hpp>

#include <tut/tut.hpp>

#include <QSet>

namespace os = qtaround::os;
namespace error = qtaround::error;
namespace util = qtaround::util;


QSet<QString> get_ftree(QString const &root);

void ensure_trees_equal(QString const &msg
                        , QSet<QString> const &before
                        , QSet<QString> const &after);

QVariant mktree(QVariantMap const &tree, QString const &root);

QVariantMap initContext(QString const &home_subdir = "");

namespace {

QVariantMap context = initContext();

inline void reinitContext(QString const &home_subdir)
{
    context = initContext(home_subdir);
};

const QVariantMap unit1_tree = {
    {"data", map({{"f1", "data1"}})},
    {"binaries", map({{"b1", "bin data"}, {"b2", "bin data 2"}})}
};

const QVariantMap unit2_tree = {
    {"unit2_data", map({{"f2", "data2"}, {"f3", "data3"}})},
    {"unit2_binaries", map({{"b1", "bin data"}, {"b2", "bin data 2"}})}
};

}

int register_unit(QString const &vault_dir, const QString &name, bool is_global);

#endif // _VAULT_TESTS_VAULT_CONTEXT_HPP_
