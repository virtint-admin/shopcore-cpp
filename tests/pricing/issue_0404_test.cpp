// Issue 0404. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0404, test_issue_0404) {
    [&] {
        SCOPED_TRACE("case 1");
        const Coupon c = coupon("FLAT200");
        EXPECT_FALSE(is_coupon_active(c, c.valid_from - 1));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        const Coupon c = coupon("FLAT200");
        EXPECT_TRUE(is_coupon_active(c, c.valid_from));
        EXPECT_FALSE(is_coupon_active(c, c.valid_until));
        EXPECT_TRUE(is_coupon_active(c, c.valid_until - 1));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_TRUE(is_coupon_active(coupon("FLAT200"), kNow));
    }();
}

}  // namespace
}  // namespace shopcore::pricing
