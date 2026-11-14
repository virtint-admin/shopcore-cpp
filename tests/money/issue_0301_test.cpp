// Issue 0301. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include <sstream>

#include "shopcore/money/money.hpp"
#include "support/fixtures.hpp"

namespace shopcore::money {
namespace {

using testing::inr;
using testing::money_of;
using testing::usd;

TEST(Issue0301, test_issue_0301) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(parse_fx_rate(" EUR/USD=1.085 ").rate_e6, 1085000);
        EXPECT_EQ(parse_fx_rate("USD/JPY=151").rate_e6, 151000000);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(parse_fx_rate("USD/INR=83.125000"), (FxRate{"USD", "INR", 83125000}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(parse_fx_rate("USDINR=83.1"), MoneyError);
        EXPECT_THROW(parse_fx_rate("USD/INR"), MoneyError);
        EXPECT_THROW(parse_fx_rate("USD/INR=0"), MoneyError);
        EXPECT_THROW(parse_fx_rate("USD/INR=1.1234567"), MoneyError);
        EXPECT_THROW(parse_fx_rate("USD/INR=8a"), MoneyError);
        EXPECT_THROW(parse_fx_rate("USD/XYZ=1.0"), UnknownCurrency);
    }();
}

}  // namespace
}  // namespace shopcore::money
