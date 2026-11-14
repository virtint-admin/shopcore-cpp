// Issue 0455. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include <algorithm>

#include "reports/report_fixtures.hpp"

namespace shopcore::reports {
namespace {

using testing::inr;

TEST(Issue0455, test_issue_0455) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(daily_sales_csv({}), "date,orders,units,net,tax\n");
    }();
    [&] {
        SCOPED_TRACE("case 2");
        const auto days = daily_sales(sample::records(), Date{2026, 11, 2}, Date{2026, 11, 3}, "INR");
        EXPECT_EQ(daily_sales_csv(days),
                  "date,orders,units,net,tax\n"
                  "2026-11-02,1,3,3997.00 INR,524.72 INR\n"
                  "2026-11-03,2,3,9997.00 INR,1149.59 INR\n");
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(inventory_snapshot_csv(inventory::Inventory{}),
                  "sku,warehouse,on_hand,reserved,available\n");
    }();
    [&] {
        SCOPED_TRACE("case 4");
        inventory::Inventory inv;
        inv.levels = {{"B", "W1", 5, 1}, {"A", "W2", 3, 0}, {"A", "W1", 2, 4}};
        EXPECT_EQ(inventory_snapshot_csv(inv),
                  "sku,warehouse,on_hand,reserved,available\n"
                  "A,W1,2,4,0\n"
                  "A,W2,3,0,3\n"
                  "B,W1,5,1,4\n");
    }();
    [&] {
        SCOPED_TRACE("case 5");
        EXPECT_EQ(top_products_csv({ProductRank{"A,B", 1, inr("1.00")}}),
                  "rank,sku,units,revenue\n1,\"A,B\",1,1.00 INR\n");
    }();
    [&] {
        SCOPED_TRACE("case 6");
        EXPECT_EQ(top_products_csv({}), "rank,sku,units,revenue\n");
    }();
}

}  // namespace
}  // namespace shopcore::reports
