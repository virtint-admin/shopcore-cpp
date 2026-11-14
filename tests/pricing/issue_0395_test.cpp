// Issue 0395. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/pricing/pricing.hpp"
#include "support/fixtures.hpp"

namespace shopcore::pricing {
namespace {

using money::Money;
using testing::inr;
using testing::seed_coupons;
using testing::ts;

const Timestamp kNow = ts("2026-11-09T12:00:00+05:30");

Coupon coupon(std::string_view code) {
    for (const Coupon& c : seed_coupons()) {
        if (c.code == code) {
            return c;
        }
    }
    throw std::runtime_error("no seed coupon " + std::string(code));
}

// Two T-shirts and a pair of earbuds: 1598.00 apparel + 2499.00 electronics.
std::vector<LineItem> basket() {
    return {LineItem{"APP-10001-BLU-M", "apparel/men/tshirts", 2, inr("799.00")},
            LineItem{"ELE-30001", "electronics/audio", 1, inr("2499.00")}};
}

std::vector<LineItem> books_only() {
    return {LineItem{"BOK-40001", "books/tech", 2, inr("899.00")}};
}

TEST(Issue0395, test_issue_0395) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(
            combine_coupons({coupon("STYLE15"), coupon("WELCOME10"), coupon("FLAT200")}, basket()),
            inr("500.00"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Coupon big = coupon("FLAT200");
        big.amount = inr("1700.00");
        EXPECT_EQ(combine_coupons({big, coupon("WELCOME10"), coupon("SHIPFREE")}, books_only()),
                  inr("1798.00"));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(combine_coupons({coupon("FLAT200")}, {}), PricingError);
        EXPECT_EQ(combine_coupons({}, basket()), inr("0.00"));
    }();
}

}  // namespace
}  // namespace shopcore::pricing
