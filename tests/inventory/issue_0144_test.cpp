// Issue 0144. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/inventory/inventory.hpp"

namespace shopcore::inventory {
namespace {

TEST(Issue0144, test_issue_0144) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(suggested_order_qty(20, 70, 30, 25), 50);
        EXPECT_EQ(suggested_order_qty(70, 70, 1, 1), 1);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(suggested_order_qty(20, 70, 100, 12), 108);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(suggested_order_qty(71, 70, 100, 12), 0);
        EXPECT_THROW(suggested_order_qty(1, 70, 100, 0), InventoryError);
    }();
}

}  // namespace
}  // namespace shopcore::inventory
