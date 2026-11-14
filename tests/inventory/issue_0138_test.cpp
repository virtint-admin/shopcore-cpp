// Issue 0138. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/inventory/inventory.hpp"

namespace shopcore::inventory {
namespace {

TEST(Issue0138, test_issue_0138) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_TRUE(needs_reorder(StockLevel{"A", "W", 80, 10}, 70));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_TRUE(needs_reorder(StockLevel{"A", "W", 100, 40}, 60));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_FALSE(needs_reorder(StockLevel{"A", "W", 81, 10}, 70));
    }();
}

}  // namespace
}  // namespace shopcore::inventory
