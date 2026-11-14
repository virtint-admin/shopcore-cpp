// Issue 0263. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "ledger/ledger_fixtures.hpp"

namespace shopcore::ledger {
namespace {

using money::Money;
using testing::inr;
using testing::seed_settlements;

TEST(Issue0263, test_issue_0263) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(gateway_fee_bp(seed_settlements()[3]), 236);
        EXPECT_EQ(gateway_fee_bp(Settlement{"p", Date{}, inr("100.00"), inr("2.00"), inr("98.00")}),
                  200);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_THROW(gateway_fee_bp(Settlement{"p", Date{}, inr("0.00"), inr("0.00"), inr("0.00")}),
                     LedgerError);
        EXPECT_THROW(
            gateway_fee_bp(Settlement{"p", Date{}, inr("1.00"), testing::usd("0.01"), inr("0.99")}),
            money::CurrencyMismatch);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(gateway_fee_bp(seed_settlements()[0]), 236);
    }();
}

}  // namespace
}  // namespace shopcore::ledger
