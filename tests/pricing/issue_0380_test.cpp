// Issue 0380. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/pricing/pricing.hpp"
#include "support/fixtures.hpp"

namespace shopcore::pricing {
namespace {

using money::Money;
using testing::inr;
using testing::kIst;
using testing::ts;

const Promotion kHappyHour{"Happy hour", 1000, 18, 21, {}};

std::vector<LineItem> basket() {
    return {LineItem{"APP-10001-BLU-M", "apparel/men/tshirts", 2, inr("799.00")},
            LineItem{"ELE-30001", "electronics/audio", 1, inr("2499.00")}};
}

TEST(Issue0380, test_issue_0380) {
    [&] {
        SCOPED_TRACE("case 1");
        QuoteContext context;
        context.coupons = testing::seed_coupons();
        context.bundles = {Bundle{"TEE-PAIR", {{"APP-10001-BLU-M", 2}}, inr("1399.00")}};
        context.now = ts("2026-11-09T12:00:00+05:30");
        context.customer_coupon_uses = {{"WELCOME10", 1}};
        EXPECT_EQ(quote_order(basket(), context), (PriceQuote{inr("4097.00"), inr("199.00"), "STYLE15",
                                                              inr("239.70"), inr("3658.30")}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Coupon fixed;
        fixed.code = "BIG500";
        fixed.kind = CouponKind::Fixed;
        fixed.amount = inr("500.00");
        fixed.min_subtotal = inr("0.00");
        fixed.valid_from = 0;
        fixed.valid_until = ts("2030-01-01T00:00:00Z");
        QuoteContext context;
        context.coupons = {fixed};
        context.bundles = {Bundle{"PAIR", {{"X", 2}}, inr("200.00")}};
        context.now = ts("2026-11-09T12:00:00Z");
        const std::vector<LineItem> lines = {LineItem{"X", "home", 2, inr("300.00")}};
        EXPECT_EQ(quote_order(lines, context),
                  (PriceQuote{inr("600.00"), inr("400.00"), "BIG500", inr("200.00"), inr("0.00")}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        QuoteContext context;
        context.coupons = testing::seed_coupons();
        context.now = ts("2026-11-09T12:00:00+05:30");
        EXPECT_EQ(quote_order(basket(), context), (PriceQuote{inr("4097.00"), inr("0.00"), "WELCOME10",
                                                              inr("300.00"), inr("3797.00")}));
    }();
    [&] {
        SCOPED_TRACE("case 4");
        QuoteContext context;
        context.now = ts("2026-11-09T12:00:00Z");
        const PriceQuote quote = quote_order(basket(), context);
        EXPECT_FALSE(quote.coupon_code.has_value());
        EXPECT_EQ(quote.total, inr("4097.00"));
        EXPECT_EQ(quote_order({}, context).total, inr("0.00"));
    }();
}

}  // namespace
}  // namespace shopcore::pricing
