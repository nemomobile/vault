#include <cor/mt.hpp>
#include <cor/util.hpp>
#include <tut/tut.hpp>

#include "tests_common.hpp"

#include <iostream>
#include <unistd.h>

namespace tut
{

struct unit_test
{
    virtual ~unit_test()
    {
    }
};

typedef test_group<unit_test> tf;
typedef tf::object object;
tf cor_unit_test("mt");

enum test_ids {
    tid_ =  1,
};

template<> template<>
void object::test<tid_>()
{
}

}
