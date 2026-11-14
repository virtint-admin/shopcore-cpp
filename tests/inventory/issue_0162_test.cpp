// Issue 0162. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/inventory/inventory.hpp"
#include "support/fixtures.hpp"

namespace shopcore::inventory {
namespace {

using testing::seed_inventory;

constexpr Timestamp kNow = 1'794'200'000;
constexpr std::int64_t kTtl = 900;

TEST(Issue0162, test_issue_0162) {
    [&] {
        SCOPED_TRACE("case 1");
        Inventory inv = seed_inventory();
        const auto ids = reserve_across_warehouses(inv, "SC-1", "ELE-30001", 60, kNow, kTtl, {});
        ASSERT_EQ(ids.size(), 2U);
        EXPECT_EQ(find_reservation(inv, ids[0])->warehouse, "BLR-1");
        EXPECT_EQ(find_reservation(inv, ids[0])->qty, 50);
        EXPECT_EQ(find_reservation(inv, ids[1])->warehouse, "BOM-1");
        EXPECT_EQ(find_reservation(inv, ids[1])->qty, 10);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Inventory inv = seed_inventory();
        const auto ids = reserve_across_warehouses(inv, "SC-1", "ELE-30001", 8, kNow, kTtl,
                                                   {"DEL-1", "XXX-1", "DEL-1"});
        ASSERT_EQ(ids.size(), 2U);
        EXPECT_EQ(find_reservation(inv, ids[0])->warehouse, "DEL-1");
        EXPECT_EQ(find_reservation(inv, ids[0])->qty, 5);
        EXPECT_EQ(find_reservation(inv, ids[1])->warehouse, "BLR-1");
        EXPECT_EQ(find_reservation(inv, ids[1])->qty, 3);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Inventory inv = seed_inventory();
        EXPECT_THROW(reserve_across_warehouses(inv, "SC-1", "ELE-30001", 76, kNow, kTtl, {}),
                     InsufficientStock);
        EXPECT_TRUE(inv.reservations.empty());
        EXPECT_THROW(reserve_across_warehouses(inv, "SC-1", "ELE-30001", 0, kNow, kTtl, {}),
                     InventoryError);
    }();
}

}  // namespace
}  // namespace shopcore::inventory
