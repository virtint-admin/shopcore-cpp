// Issue 0483. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0483, test_issue_0483) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(growth_bp(inr("1.00"), inr("3.00")), -6667);
        EXPECT_EQ(growth_bp(inr("-50.00"), inr("-100.00")), 5000);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(growth_bp(inr("1100.00"), inr("1000.00")), 1000);
        EXPECT_EQ(growth_bp(inr("900.00"), inr("1000.00")), -1000);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_FALSE(growth_bp(inr("10.00"), inr("0.00")).has_value());
        EXPECT_THROW(growth_bp(inr("10.00"), testing::usd("1.00")), money::CurrencyMismatch);
    }();
}

}  // namespace
}  // namespace shopcore::reports
