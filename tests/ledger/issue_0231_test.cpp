// Issue 0231. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "ledger/ledger_fixtures.hpp"

namespace shopcore::ledger {
namespace {

using money::Money;
using testing::inr;

const Date kNov2{2026, 11, 2};
const Date kNov3{2026, 11, 3};

TEST(Issue0231, test_issue_0231) {
    [&] {
        SCOPED_TRACE("case 1");
        const auto rows = trial_balance(sample::small(), kNov2);
        ASSERT_EQ(rows.size(), 5U);
        EXPECT_EQ(rows[0].account, "assets:bank");
        EXPECT_EQ(rows[1].account, "expenses:gateway_fees");
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(
            trial_balance(sample::small(), kNov3),
            (std::vector<TrialBalanceRow>{{"assets:bank", inr("1639.33"), inr("0.00")},
                                          {"assets:receivable:gateway", inr("2948.82"), inr("0.00")},
                                          {"expenses:gateway_fees", inr("39.62"), inr("0.00")},
                                          {"liabilities:gst_output", inr("0.00"), inr("679.77")},
                                          {"revenue:sales", inr("0.00"), inr("3899.00")},
                                          {"revenue:shipping", inr("0.00"), inr("49.00")}}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_TRUE(trial_balance(Ledger{}, kNov3).empty());
        EXPECT_TRUE(trial_balance(sample::small(), Date{2026, 1, 1}).empty());
    }();
}

}  // namespace
}  // namespace shopcore::ledger
