// Issue 0150. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/inventory/inventory.hpp"
#include "support/fixtures.hpp"

namespace shopcore::inventory {
namespace {

using testing::seed_inventory;

constexpr Timestamp kNow = 1'794'200'000;
constexpr std::int64_t kTtl = 900;

TEST(Issue0150, test_issue_0150) {
    [&] {
        SCOPED_TRACE("case 1");
        Inventory inv = seed_inventory();
        create_reservation(inv, "SC-1", "ELE-30001", "BLR-1", 5, kNow, kTtl);
        create_reservation(inv, "SC-2", "ELE-30001", "BLR-1", 5, kNow, kTtl * 2);
        EXPECT_EQ(expire_reservations(inv, kNow + kTtl), 1);
        EXPECT_EQ(find_reservation(inv, 1)->state, ReservationState::Expired);
        EXPECT_EQ(find_reservation(inv, 2)->state, ReservationState::Active);
        EXPECT_EQ(find_level(inv, "ELE-30001", "BLR-1")->reserved, 15);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Inventory inv = seed_inventory();
        create_reservation(inv, "SC-1", "ELE-30001", "BLR-1", 5, kNow, kTtl);
        EXPECT_EQ(expire_reservations(inv, kNow + kTtl - 1), 0);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Inventory inv = seed_inventory();
        const auto id = create_reservation(inv, "SC-1", "ELE-30001", "BLR-1", 5, kNow, kTtl);
        release_reservation(inv, id);
        EXPECT_EQ(expire_reservations(inv, kNow + kTtl * 10), 0);
        EXPECT_EQ(find_reservation(inv, id)->state, ReservationState::Released);
    }();
}

}  // namespace
}  // namespace shopcore::inventory
