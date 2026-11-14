// Issue 0199. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/inventory/inventory.hpp"
#include "support/fixtures.hpp"

namespace shopcore::inventory {
namespace {

using testing::seed_inventory;

TEST(Issue0199, test_issue_0199) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(plan_transfer(seed_inventory(), "ELE-30001", "BLR-1", "DEL-1", 20),
                  (TransferOrder{"ELE-30001", "BLR-1", "DEL-1", 20, 0, 0, TransferStatus::Planned}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_NO_THROW(plan_transfer(seed_inventory(), "ELE-30001", "BLR-1", "DEL-1", 50));
        EXPECT_THROW(plan_transfer(seed_inventory(), "ELE-30001", "BLR-1", "DEL-1", 51),
                     InsufficientStock);
        EXPECT_THROW(plan_transfer(seed_inventory(), "ELE-30002", "BLR-1", "DEL-1", 1),
                     InsufficientStock);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(plan_transfer(seed_inventory(), "ELE-30001", "BLR-1", "BLR-1", 1), InventoryError);
        EXPECT_THROW(plan_transfer(seed_inventory(), "ELE-30001", "BLR-1", "DEL-1", 0), InventoryError);
    }();
}

}  // namespace
}  // namespace shopcore::inventory
