#ifndef _VAULT_TRANSFER_HPP_
#define _VAULT_TRANSFER_HPP_

#include "util.hpp"
#include "subprocess.hpp"

#include <QString>
#include <QVariant>
#include <QList>

enum class Io { Exec, Options, OnProgress, EstSize, Dst, EOE };
template <> struct StructTraits<Io>
{
    typedef std::tuple<QString, QStringList
                       , std::function<void(QVariantMap &&)>
                       , long, QString> type;

    STRUCT_NAMES(Io, "Exec", "Options", "OnProgress", "EstSize", "Dst");
};

typedef Struct<Io> IoCmd;

namespace vault { class Vault; }

class CardTransfer : public QObject
{
    Q_OBJECT
    Q_ENUMS(Actions)
    Q_PROPERTY(QString src READ getSrc);
    Q_PROPERTY(QString dst READ getDst);
    Q_PROPERTY(Action action READ getAction);
    Q_PROPERTY(double spaceFree READ getSpace);
    Q_PROPERTY(double spaceRequired READ getRequired);

public:
    enum Action { Export, Import, ActionsEnd };

    CardTransfer()
        : vault_(nullptr)
        , action_(ActionsEnd)
        , space_free_(0)
        , space_required_(0)
    {}

    typedef std::function<void(QVariantMap&&)> progressCallback;
    void init(vault::Vault *, Action, QString const &);
    void execute(progressCallback);

signals:
    void vaultChanged();
private:
    static subprocess::Process doIO(IoCmd const &info);
    static void validateDump(QString const &archive, QVariantMap const &err);
    void estimateSpace();
    void exportStorage(progressCallback);
    void importStorage(progressCallback);

    inline QString getSrc() const { return src_; }
    inline QString getDst() const { return dst_; }
    inline Action getAction() const { return action_; }
    inline double getSpace() const { return space_free_; }
    inline double getRequired() const { return space_required_; }
    vault::Vault *getVault();
    void invalidateVault();

    vault::Vault *vault_;
    Action action_;
    QString src_;
    QString dst_;
    double space_free_;
    double space_required_;
};

#endif // _VAULT_TRANSFER_HPP_
