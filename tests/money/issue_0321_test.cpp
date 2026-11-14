// Issue 0321. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include <sstream>

#include "shopcore/money/money.hpp"
#include "support/fixtures.hpp"

namespace shopcore::money {
namespace {

using testing::inr;
using testing::money_of;
using testing::usd;

TEST(Issue0321, test_issue_0321) {
    [&] {
        SCOPED_TRACE("case 1");
        const FxRate rate{"USD", "INR", 83250000};
        EXPECT_EQ(convert(usd("100.00"), rate, RoundingMode::HalfUp), inr("8325.00"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(convert(usd("10.00"), FxRate{"USD", "JPY", 151400000}, RoundingMode::HalfUp),
                  (Money{1514, "JPY"}));
        EXPECT_EQ(convert(Money{1000, "KWD"}, FxRate{"KWD", "USD", 3250000}, RoundingMode::HalfUp),
                  usd("3.25"));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        const FxRate rate{"EUR", "INR", 90326250};
        EXPECT_EQ(convert(money_of("100.00 EUR"), rate, RoundingMode::HalfUp), inr("9032.63"));
        EXPECT_EQ(convert(money_of("100.00 EUR"), rate, RoundingMode::HalfEven), inr("9032.62"));
    }();
    [&] {
        SCOPED_TRACE("case 4");
        EXPECT_EQ(convert(inr("5.00"), FxRate{"INR", "INR", kRateScale}, RoundingMode::HalfUp),
                  inr("5.00"));
        EXPECT_THROW(
            convert(money_of("5.00 EUR"), FxRate{"USD", "INR", 83250000}, RoundingMode::HalfUp),
            CurrencyMismatch);
        EXPECT_THROW(convert(usd("5.00"), FxRate{"USD", "INR", 0}, RoundingMode::HalfUp), MoneyError);
    }();
    [&] {
        SCOPED_TRACE("case 5");
        const FxRate rate{"USD", "INR", 83250000};
        EXPECT_EQ(convert(inr("1000.00"), rate, RoundingMode::HalfUp), usd("12.01"));
        EXPECT_EQ(convert(inr("1000.00"), rate, RoundingMode::Ceiling), usd("12.02"));
    }();
}

}  // namespace
}  // namespace shopcore::money
