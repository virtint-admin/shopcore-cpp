// Issue 0139. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/inventory/inventory.hpp"

namespace shopcore::inventory {
namespace {

TEST(Issue0139, test_issue_0139) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(economic_order_quantity(100, 200, 1), 200);
        EXPECT_EQ(economic_order_quantity(0, 200, 1), 0);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(economic_order_quantity(1200, 50000, 2000), 245);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(economic_order_quantity(100, 200, 0), InventoryError);
        EXPECT_THROW(economic_order_quantity(-1, 200, 5), InventoryError);
    }();
}

}  // namespace
}  // namespace shopcore::inventory
