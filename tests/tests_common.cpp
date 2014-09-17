#include <vault/vault.hpp>

int register_unit(QString const &vault_dir, const QString &name, bool is_global)
{
    QVariantMap data = {{"action", "register"},
                        {"data", "name=" + name + ",group=group1,"
                                 + "script=./" + name + "_vault_test"}};
    if (is_global) {
        data["global"] = true;
    } else {
        data["vault"] = vault_dir;
    }
    return vault::Vault::execute(data);
};
