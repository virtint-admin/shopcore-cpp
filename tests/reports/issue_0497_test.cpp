// Issue 0497. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0497, test_issue_0497) {
    [&] {
        SCOPED_TRACE("case 1");
        const auto weeks = weekly_totals(daily_sales(sample::records(), kNov1, kNov5, "INR"));
        EXPECT_EQ(weeks, (std::vector<std::pair<Date, Money>>{{Date{2026, 10, 26}, inr("0.00")},
                                                              {Date{2026, 11, 2}, inr("15041.00")}}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        const auto weeks =
            weekly_totals({DailySales{Date{2026, 11, 15}, 1, 1, inr("5.00"), inr("0.00")},
                           DailySales{Date{2026, 11, 16}, 1, 1, inr("7.00"), inr("0.00")}});
        EXPECT_EQ(weeks, (std::vector<std::pair<Date, Money>>{{Date{2026, 11, 9}, inr("5.00")},
                                                              {Date{2026, 11, 16}, inr("7.00")}}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_TRUE(weekly_totals({}).empty());
    }();
}

}  // namespace
}  // namespace shopcore::reports
