// Issue 0393. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0393, test_issue_0393) {
    [&] {
        SCOPED_TRACE("case 1");
        Coupon flat = coupon("FLAT200");
        EXPECT_EQ(redeem_coupon(flat), 1);
        EXPECT_EQ(redeem_coupon(flat), 0);
        EXPECT_EQ(flat.uses, 1000);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Coupon welcome = coupon("WELCOME10");
        EXPECT_EQ(redeem_coupon(welcome), -1);
        EXPECT_EQ(welcome.uses, 1);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Coupon flat = coupon("FLAT200");
        flat.uses = 1000;
        EXPECT_THROW(redeem_coupon(flat), PricingError);
    }();
}

}  // namespace
}  // namespace shopcore::pricing
