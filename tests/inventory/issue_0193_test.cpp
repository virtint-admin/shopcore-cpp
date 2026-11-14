// Issue 0193. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/inventory/inventory.hpp"
#include "support/fixtures.hpp"

namespace shopcore::inventory {
namespace {

using testing::inr;
using testing::seed_inventory;

TEST(Issue0193, test_issue_0193) {
    [&] {
        SCOPED_TRACE("case 1");
        Inventory inv = seed_inventory();
        EXPECT_THROW(adjust_stock(inv, "ELE-30001", "BLR-1", -51), InventoryError);
        EXPECT_THROW(adjust_stock(inv, "ELE-30001", "XXX-9", 1), InventoryError);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Inventory inv = seed_inventory();
        EXPECT_EQ(adjust_stock(inv, "ELE-30001", "BLR-1", -50), 10);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Inventory inv = seed_inventory();
        EXPECT_EQ(adjust_stock(inv, "ELE-30001", "BLR-1", -45), 15);
        EXPECT_EQ(adjust_stock(inv, "ELE-30001", "BLR-1", 5), 20);
    }();
}

}  // namespace
}  // namespace shopcore::inventory
