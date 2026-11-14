// Issue 0237. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "ledger/ledger_fixtures.hpp"

namespace shopcore::ledger {
namespace {

using money::Money;
using testing::inr;

const Date kNov2{2026, 11, 2};
const Date kNov3{2026, 11, 3};

TEST(Issue0237, test_issue_0237) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(net_income(sample::small(), Date{2026, 11, 1}, kNov3), inr("3908.38"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Ledger ledger = sample::small();
        EXPECT_EQ(net_income(ledger, kNov3, kNov3), inr("2499.00"));
        record_refund(ledger, kNov3, "pay_N4d5e6f", inr("499.00"), inr("89.82"));
        EXPECT_EQ(net_income(ledger, kNov3, kNov3), inr("2000.00"));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(net_income(sample::small(), kNov3, kNov2), LedgerError);
    }();
}

}  // namespace
}  // namespace shopcore::ledger
