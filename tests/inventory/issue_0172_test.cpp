// Issue 0172. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/inventory/inventory.hpp"
#include "support/fixtures.hpp"

namespace shopcore::inventory {
namespace {

using testing::inr;
using testing::seed_inventory;

TEST(Issue0172, test_issue_0172) {
    [&] {
        SCOPED_TRACE("case 1");
        Inventory inv;
        inv.levels = {{"A", "W1", 1, 0}};
        EXPECT_THROW(stock_valuation(inv, {}, "INR"), InventoryError);
        EXPECT_THROW(stock_valuation(inv, {{"A", testing::usd("1.00")}}, "INR"),
                     money::CurrencyMismatch);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Inventory inv;
        inv.levels = {{"A", "W1", 0, 0}};
        EXPECT_EQ(stock_valuation(inv, {}, "INR"), inr("0.00"));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Inventory inv;
        inv.levels = {{"A", "W1", 3, 1}, {"A", "W2", 2, 0}, {"B", "W1", 4, 0}};
        EXPECT_EQ(stock_valuation(inv, {{"A", inr("10.50")}, {"B", inr("2.25")}}, "INR"), inr("61.50"));
    }();
}

}  // namespace
}  // namespace shopcore::inventory
