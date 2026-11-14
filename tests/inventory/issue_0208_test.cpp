// Issue 0208. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/inventory/inventory.hpp"
#include "support/fixtures.hpp"

namespace shopcore::inventory {
namespace {

using testing::seed_inventory;

TEST(Issue0208, test_issue_0208) {
    [&] {
        SCOPED_TRACE("case 1");
        Inventory inv;
        inv.levels = {{"A", "W1", 30, 0}, {"A", "W2", 4, 0}, {"A", "W3", 0, 0}};
        EXPECT_EQ(rebalance_suggestions(inv, "A", 10),
                  (std::vector<TransferOrder>{{"A", "W1", "W3", 10, 0, 0, TransferStatus::Planned},
                                              {"A", "W1", "W2", 6, 0, 0, TransferStatus::Planned}}));
        EXPECT_TRUE(rebalance_suggestions(inv, "B", 10).empty());
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(rebalance_suggestions(seed_inventory(), "ELE-30001", 15),
                  (std::vector<TransferOrder>{
                      {"ELE-30001", "BLR-1", "DEL-1", 10, 0, 0, TransferStatus::Planned}}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Inventory inv;
        inv.levels = {{"A", "W1", 15, 0}, {"A", "W2", 13, 0}, {"A", "W3", 0, 0}, {"B", "W4", 0, 0}};
        EXPECT_EQ(rebalance_suggestions(inv, "A", 10),
                  (std::vector<TransferOrder>{{"A", "W1", "W3", 5, 0, 0, TransferStatus::Planned},
                                              {"A", "W2", "W3", 3, 0, 0, TransferStatus::Planned}}));
    }();
}

}  // namespace
}  // namespace shopcore::inventory
