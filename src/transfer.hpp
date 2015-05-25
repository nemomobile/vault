#ifndef _VAULT_TRANSFER_HPP_
#define _VAULT_TRANSFER_HPP_
/**
 * @file transfer.hpp
 * @brief API to support storage transfer
 * @author Denis Zalevskiy <denis.zalevskiy@jolla.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <qtaround/util.hpp>
#include <qtaround/subprocess.hpp>
#include <cor/util.hpp>

#include <QString>
#include <QVariant>
#include <QList>

enum class Io { Exec, Options, OnProgress, EstSize, Dst, Last_ = Dst };
typedef Record<Io, QString, QStringList
               , std::function<void(QVariantMap &&)>
               , long, QString> IoCmd;

template <> struct RecordTraits<IoCmd>
{
    RECORD_FIELD_NAMES(IoCmd, "Exec", "Options", "OnProgress", "EstSize", "Dst");
};

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

    inline QString getSrc() const { return src_; }
    inline QString getDst() const { return dst_; }
    inline Action getAction() const { return action_; }
    inline double getSpace() const { return space_free_; }
    inline double getRequired() const { return space_required_; }

signals:
    void vaultChanged();
private:
    static qtaround::subprocess::Process doIO(IoCmd const &info);
    static void validateDump(QString const &archive, QVariantMap const &err);
    void estimateSpace();
    void exportStorage(progressCallback);
    void importStorage(progressCallback);

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
