// Issue 0176. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/inventory/inventory.hpp"
#include "support/fixtures.hpp"

namespace shopcore::inventory {
namespace {

using testing::inr;
using testing::seed_inventory;

TEST(Issue0176, test_issue_0176) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(warehouses_with_stock(seed_inventory(), "ELE-30001", 20),
                  (std::vector<std::string>{"BLR-1", "BOM-1"}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(warehouses_with_stock(seed_inventory(), "ELE-30001", 1),
                  (std::vector<std::string>{"BLR-1", "BOM-1", "DEL-1"}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Inventory inv;
        inv.levels = {{"A", "W2", 5, 0}, {"A", "W1", 5, 0}, {"A", "W3", 0, 0}};
        EXPECT_EQ(warehouses_with_stock(inv, "A", 0), (std::vector<std::string>{"W1", "W2"}));
    }();
}

}  // namespace
}  // namespace shopcore::inventory
