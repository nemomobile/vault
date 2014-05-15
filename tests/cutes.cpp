#include <util.hpp>
#include <tut/tut.hpp>
#include "tests_common.hpp"

#include <QDebug>
#include <QVariant>

namespace tut
{

struct cutes_test
{
    virtual ~cutes_test()
    {
    }
};

typedef test_group<cutes_test> tf;
typedef tf::object object;
tf vault_cutes_test("cutes");

enum test_ids {
    tid_visit =  1,
};

template<> template<>
void object::test<tid_visit>()
{
    QVariantMap src = {
        {"b1", true},
        {"m1", map({{"s1", "Str 1"}})},
        {"l1", list({false, "List str"})}
    };
    QStringList expected = {"///"
                            , "QString//QString/b1/bool/true"
                            , "QString//QString/l1"
                            , "QString//QString/m1"
                            , "QString/l1/int/0/bool/false"
                            , "QString/l1/int/1/QString/List str"
                            , "QString/m1/QString/s1/QString/Str 1"};

    QStringList dst;
    auto fn = [&dst](QVariant const &ctx, QVariant const &key
                  , QVariant const &data) {
        auto k = str(key);
        QStringList v;
        v << ctx.typeName() << str(ctx) << key.typeName() << k; 
        if (!(hasType(data, QMetaType::QVariantMap)
              || hasType(data, QMetaType::QVariantList)))
            v << data.typeName() << str(data);
        dst << v.join("/");
        qDebug() << "VISIT" << ctx << key << data;
        return k;
    };
    util::visit(fn, src, QVariant());
    dst.sort();
    expected.sort();
    qDebug() << dst;
    ensure_eq("Visited items", dst, expected);
}

}
