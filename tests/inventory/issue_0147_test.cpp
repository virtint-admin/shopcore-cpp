// Issue 0147. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/inventory/inventory.hpp"
#include "support/fixtures.hpp"

namespace shopcore::inventory {
namespace {

using testing::seed_inventory;

constexpr Timestamp kNow = 1'794'200'000;
constexpr std::int64_t kTtl = 900;

TEST(Issue0147, test_issue_0147) {
    [&] {
        SCOPED_TRACE("case 1");
        Inventory inv = seed_inventory();
        create_reservation(inv, "SC-1", "ELE-30001", "BLR-1", 5, kNow, kTtl);
        create_reservation(inv, "SC-1", "GRO-50001", "BLR-1", 2, kNow, kTtl);
        create_reservation(inv, "SC-2", "ELE-30001", "BLR-1", 1, kNow, kTtl);
        EXPECT_EQ(release_order_reservations(inv, "SC-1"), 2);
        EXPECT_EQ(find_level(inv, "ELE-30001", "BLR-1")->reserved, 11);
        EXPECT_EQ(find_level(inv, "GRO-50001", "BLR-1")->reserved, 20);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Inventory inv = seed_inventory();
        const auto id = create_reservation(inv, "SC-1", "ELE-30001", "BLR-1", 5, kNow, kTtl);
        create_reservation(inv, "SC-1", "ELE-30001", "BOM-1", 3, kNow, kTtl);
        fulfil_reservation(inv, id);
        EXPECT_EQ(release_order_reservations(inv, "SC-1"), 1);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Inventory inv = seed_inventory();
        EXPECT_EQ(release_order_reservations(inv, "SC-404"), 0);
    }();
}

}  // namespace
}  // namespace shopcore::inventory
