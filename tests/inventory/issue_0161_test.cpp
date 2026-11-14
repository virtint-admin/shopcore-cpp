// Issue 0161. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/inventory/inventory.hpp"
#include "support/fixtures.hpp"

namespace shopcore::inventory {
namespace {

using testing::seed_inventory;

constexpr Timestamp kNow = 1'794'200'000;
constexpr std::int64_t kTtl = 900;

TEST(Issue0161, test_issue_0161) {
    [&] {
        SCOPED_TRACE("case 1");
        Inventory inv = seed_inventory();
        const auto id = create_reservation(inv, "SC-1", "ELE-30001", "BLR-1", 5, kNow, kTtl);
        EXPECT_EQ(extend_reservation(inv, id, kNow + 100, 600), kNow + kTtl + 600);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Inventory inv = seed_inventory();
        const auto id = create_reservation(inv, "SC-1", "ELE-30001", "BLR-1", 5, kNow, kTtl);
        EXPECT_EQ(extend_reservation(inv, id, kNow + 5000, 600), kNow + 5600);
        EXPECT_EQ(find_reservation(inv, id)->expires_at, kNow + 5600);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Inventory inv = seed_inventory();
        const auto id = create_reservation(inv, "SC-1", "ELE-30001", "BLR-1", 5, kNow, kTtl);
        EXPECT_THROW(extend_reservation(inv, id, kNow, 0), InventoryError);
        release_reservation(inv, id);
        EXPECT_THROW(extend_reservation(inv, id, kNow, 60), InventoryError);
        EXPECT_THROW(extend_reservation(inv, 99, kNow, 60), InventoryError);
    }();
}

}  // namespace
}  // namespace shopcore::inventory
