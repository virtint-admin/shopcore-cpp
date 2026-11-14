// Issue 0133. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/inventory/inventory.hpp"

namespace shopcore::inventory {
namespace {

TEST(Issue0133, test_issue_0133) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(average_daily_demand({2, 2}), 2);
        EXPECT_EQ(average_daily_demand({}), 0);
        EXPECT_EQ(average_daily_demand({0, 0, 1}), 1);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(average_daily_demand({3, 4, 4}), 4);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(average_daily_demand({1, -1}), InventoryError);
    }();
}

}  // namespace
}  // namespace shopcore::inventory
