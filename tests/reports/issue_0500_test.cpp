// Issue 0500. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "orders/order_fixtures.hpp"
#include "reports/report_fixtures.hpp"

namespace shopcore::reports {
namespace {

using money::Money;
using testing::inr;
using testing::kIst;

const Date kNov1{2026, 11, 1};
const Date kNov5{2026, 11, 5};

TEST(Issue0500, test_issue_0500) {
    [&] {
        SCOPED_TRACE("case 1");
        orders::Order order = orders::sample::delivered();
        order.lines[0].refunded_qty = 1;
        order.lines[1].refunded_qty = 1;
        const auto records = sale_records_from_order(order, "KA", kIst);
        ASSERT_EQ(records.size(), 1U);
        EXPECT_EQ(records[0].qty, 1);
        EXPECT_EQ(records[0].net, inr("749.00"));
        EXPECT_EQ(records[0].tax, inr("37.45"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        const auto records = sale_records_from_order(orders::sample::delivered(), "KA", kIst);
        ASSERT_EQ(records.size(), 2U);
        EXPECT_EQ(records[0],
                  (SaleRecord{"SC-20261109-000042", Date{2026, 11, 9}, "APP-10001-BLU-M",
                              "apparel/men/tshirts", "KA", 2, inr("1498.00"), inr("74.90")}));
        EXPECT_EQ(records[1].net, inr("2499.00"));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_TRUE(sale_records_from_order(orders::sample::pending(), "KA", kIst).empty());
        orders::Order cancelled = orders::sample::paid();
        orders::cancel_order(cancelled, "x", orders::sample::kT0 + 200);
        EXPECT_TRUE(sale_records_from_order(cancelled, "KA", kIst).empty());
        orders::Order late = orders::sample::paid();
        late.paid_at = testing::ts("2026-11-09T20:00:00Z");
        EXPECT_EQ(sale_records_from_order(late, "KA", kIst)[0].date, (Date{2026, 11, 10}));
        EXPECT_EQ(sale_records_from_order(late, "KA", 0)[0].date, (Date{2026, 11, 9}));
    }();
}

}  // namespace
}  // namespace shopcore::reports
