// Issue 0194. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/inventory/inventory.hpp"
#include "support/fixtures.hpp"

namespace shopcore::inventory {
namespace {

using testing::inr;
using testing::seed_inventory;

TEST(Issue0194, test_issue_0194) {
    [&] {
        SCOPED_TRACE("case 1");
        Inventory inv = seed_inventory();
        EXPECT_EQ(write_off(inv, "GRO-50001", "BLR-1", 80), 20);
        EXPECT_THROW(write_off(inv, "GRO-50001", "BLR-1", 1), InsufficientStock);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Inventory inv = seed_inventory();
        EXPECT_EQ(write_off(inv, "GRO-50001", "BLR-1", 7), 93);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Inventory inv = seed_inventory();
        EXPECT_THROW(write_off(inv, "GRO-50001", "BLR-1", 0), InventoryError);
        EXPECT_THROW(write_off(inv, "GRO-50001", "DEL-1", 1), InventoryError);
    }();
}

}  // namespace
}  // namespace shopcore::inventory
