// Issue 0191. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/inventory/inventory.hpp"
#include "support/fixtures.hpp"

namespace shopcore::inventory {
namespace {

using testing::inr;
using testing::seed_inventory;

TEST(Issue0191, test_issue_0191) {
    [&] {
        SCOPED_TRACE("case 1");
        Inventory inv = seed_inventory();
        EXPECT_EQ(receive_stock(inv, "ELE-30002", "BLR-1", 4), 4);
        EXPECT_EQ(find_level(inv, "ELE-30002", "BLR-1"), (StockLevel{"ELE-30002", "BLR-1", 4, 0}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Inventory inv = seed_inventory();
        EXPECT_EQ(receive_stock(inv, "ELE-30002", "DEL-1", 6), 15);
        EXPECT_EQ(find_level(inv, "ELE-30002", "DEL-1")->on_hand, 15);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Inventory inv = seed_inventory();
        EXPECT_THROW(receive_stock(inv, "ELE-30002", "DEL-1", 0), InventoryError);
    }();
}

}  // namespace
}  // namespace shopcore::inventory
