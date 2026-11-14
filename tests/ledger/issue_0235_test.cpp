// Issue 0235. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "ledger/ledger_fixtures.hpp"

namespace shopcore::ledger {
namespace {

using money::Money;
using testing::inr;

const Date kNov2{2026, 11, 2};
const Date kNov3{2026, 11, 3};

TEST(Issue0235, test_issue_0235) {
    [&] {
        SCOPED_TRACE("case 1");
        auto rows = trial_balance(sample::small(), kNov3);
        rows[0].debit = inr("1639.34");
        EXPECT_FALSE(trial_balance_is_even(rows));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_TRUE(trial_balance_is_even({}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_TRUE(trial_balance_is_even(trial_balance(sample::small(), kNov3)));
    }();
}

}  // namespace
}  // namespace shopcore::ledger
