// Issue 0229. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "ledger/ledger_fixtures.hpp"

namespace shopcore::ledger {
namespace {

using money::Money;
using testing::inr;

const Date kNov2{2026, 11, 2};
const Date kNov3{2026, 11, 3};

TEST(Issue0229, test_issue_0229) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(account_balance(sample::small(), "assets", kNov3), inr("4588.15"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        const Ledger ledger = sample::small();
        EXPECT_EQ(account_balance(ledger, "revenue:sales", kNov2), inr("-1400.00"));
        EXPECT_EQ(account_balance(ledger, "revenue:sales", Date{2026, 11, 1}), inr("0.00"));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        const auto lines = account_statement(sample::small(), "assets", kNov3, kNov3);
        ASSERT_EQ(lines.size(), 1U);
        EXPECT_EQ(lines[0].balance, inr("4588.15"));
    }();
    [&] {
        SCOPED_TRACE("case 4");
        EXPECT_EQ(normal_balance(sample::small(), "liabilities", kNov3), inr("679.77"));
        EXPECT_EQ(normal_balance(sample::small(), "expenses:gateway_fees", kNov3), inr("39.62"));
    }();
    [&] {
        SCOPED_TRACE("case 5");
        EXPECT_EQ(normal_balance(sample::small(), "revenue:sales", kNov3), inr("3899.00"));
    }();
    [&] {
        SCOPED_TRACE("case 6");
        EXPECT_THROW(normal_balance(sample::small(), "income", kNov3), LedgerError);
    }();
}

}  // namespace
}  // namespace shopcore::ledger
