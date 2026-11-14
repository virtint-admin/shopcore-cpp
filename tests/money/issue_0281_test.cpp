// Issue 0281. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include <sstream>

#include "shopcore/money/money.hpp"
#include "support/fixtures.hpp"

namespace shopcore::money {
namespace {

using testing::inr;
using testing::usd;

TEST(Issue0281, test_issue_0281) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(format_display(Money{123456789, "INR"}), "₹12,34,567.89");
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(format_display(Money{-500, "INR"}), "-₹5.00");
        EXPECT_EQ(format_display(Money{99900, "INR"}), "₹999.00");
        EXPECT_EQ(format_display(Money{100000, "INR"}), "₹1,000.00");
        EXPECT_EQ(format_display(Money{500, "JPY"}), "¥500");
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(format_display(Money{123456750, "USD"}), "$1,234,567.50");
    }();
}

}  // namespace
}  // namespace shopcore::money
