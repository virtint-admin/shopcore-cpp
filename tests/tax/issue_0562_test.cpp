// Issue 0562. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/tax/tax.hpp"
#include "support/fixtures.hpp"

namespace shopcore::tax {
namespace {

using money::Money;
using testing::inr;

TEST(Issue0562, test_issue_0562) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(gst_rate_for_hsn("9403", inr("5499.00")), 1800);
        EXPECT_EQ(gst_rate_for_hsn("85044010", inr("1.00")), 1800);
        EXPECT_THROW(gst_rate_for_hsn("61", inr("1.00")), TaxError);
        EXPECT_THROW(gst_rate_for_hsn("6109", testing::usd("10.00")), money::CurrencyMismatch);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(gst_rate_for_hsn("4901", inr("899.00")), 0);
        EXPECT_EQ(gst_rate_for_hsn("0902", inr("349.00")), 500);
        EXPECT_EQ(gst_rate_for_hsn("5007", inr("4999.00")), 500);
        EXPECT_EQ(gst_rate_for_hsn("8518", inr("2499.00")), 1800);
        EXPECT_EQ(gst_rate_for_hsn("7323", inr("599.00")), 1200);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(gst_rate_for_hsn("6109", inr("799.00")), 500);
        EXPECT_EQ(gst_rate_for_hsn("6205", inr("1000.00")), 500);
        EXPECT_EQ(gst_rate_for_hsn("6304", inr("1000.01")), 1200);
    }();
}

}  // namespace
}  // namespace shopcore::tax
