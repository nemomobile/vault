#include <util.hpp>
#include <os.hpp>
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
    tid_visit =  1
    , tid_zip
    , tid_mountpoint
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
        return k;
    };
    util::visit(fn, src, QVariant());
    dst.sort();
    expected.sort();
    ensure_eq("Visited items", dst, expected);
}

template<> template<>
void object::test<tid_zip>()
{
    QStringList l1, l2;
    l1 << "A";
    l2 << "a";
    auto res = util::zip(l1, l2);
    ensure_eq("Single item", res.size(), 1);
    l1 << "B";
    res = util::zip(l1, l2);
    ensure_eq("2 items, 2nd empty", res.size(), 2);
    l2 << "b";
    res = util::zip(l1, l2);
    ensure_eq("2 items", res.size(), 2);
    l2 << "c";
    res = util::zip(l1, l2);
    ensure_eq("2 items, w/o 3rd", res.size(), 2);

}

template<> template<>
void object::test<tid_mountpoint>()
{
    auto res = os::mountpoint(".");
    ensure_ne("There should be some mountpoint", res.size(), 0);
    auto s = os::stat(".", {{"fields", "m"}});
    res = s["mount_point"];
    if (res != "?")
        ensure_ne("There should be some mountpoint if it is not ?", res.size(), 0);
}


}
