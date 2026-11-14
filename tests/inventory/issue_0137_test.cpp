// Issue 0137. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/inventory/inventory.hpp"

namespace shopcore::inventory {
namespace {

TEST(Issue0137, test_issue_0137) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(safety_stock(3, 2, 5, 4), 0);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(safety_stock(10, 7, 6, 5), 40);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(safety_stock(-1, 1, 1, 1), InventoryError);
        EXPECT_THROW(safety_stock(1, 1, 1, -1), InventoryError);
    }();
}

}  // namespace
}  // namespace shopcore::inventory
