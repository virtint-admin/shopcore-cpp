// Issue 0153. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/inventory/inventory.hpp"
#include "support/fixtures.hpp"

namespace shopcore::inventory {
namespace {

using testing::seed_inventory;

constexpr Timestamp kNow = 1'794'200'000;
constexpr std::int64_t kTtl = 900;

TEST(Issue0153, test_issue_0153) {
    [&] {
        SCOPED_TRACE("case 1");
        Inventory inv = seed_inventory();
        const auto id = create_reservation(inv, "SC-1", "ELE-30001", "BLR-1", 5, kNow, kTtl);
        create_reservation(inv, "SC-1", "ELE-30001", "BOM-1", 3, kNow, kTtl);
        release_reservation(inv, id);
        EXPECT_EQ(reserved_qty_for_order(inv, "SC-1"), 3);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(reserved_qty_for_order(seed_inventory(), "SC-1"), 0);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Inventory inv = seed_inventory();
        create_reservation(inv, "SC-1", "ELE-30001", "BLR-1", 5, kNow, kTtl);
        create_reservation(inv, "SC-1", "ELE-30001", "BOM-1", 3, kNow, kTtl);
        EXPECT_EQ(reserved_qty_for_order(inv, "SC-1"), 8);
    }();
}

}  // namespace
}  // namespace shopcore::inventory
