// Issue 0558. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/cart/cart.hpp"
#include "support/fixtures.hpp"

namespace shopcore::cart {
namespace {

using testing::inr;
using testing::ts;

const Timestamp kNow = ts("2026-11-09T12:00:00+05:30");

CartContext context() {
    CartContext ctx;
    ctx.products = testing::seed_products();
    ctx.tax_rules = testing::seed_tax_rules();
    ctx.coupons = testing::seed_coupons();
    ctx.region = "KA";
    ctx.tax_date = Date{2026, 11, 9};
    ctx.now = kNow;
    ctx.shipping_fee = inr("49.00");
    ctx.free_shipping_threshold = inr("999.00");
    return ctx;
}

Cart basket() {
    Cart cart = new_cart("c-1", "cust-9", "INR", kNow);
    add_item(cart, "APP-10001-BLU-M", 2, inr("799.00"), kNow);
    add_item(cart, "ELE-30001", 1, inr("2499.00"), kNow);
    return cart;
}

Cart one_book() {
    Cart cart = new_cart("c-2", "", "INR", kNow);
    add_item(cart, "BOK-40002", 1, inr("499.00"), kNow);
    return cart;
}

TEST(Issue0558, test_issue_0558) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(cart_totals(new_cart("c", "", "INR", kNow), context()),
                  (CartSummary{inr("0.00"), inr("0.00"), std::nullopt, inr("0.00"), inr("0.00"),
                               inr("0.00")}));
        CartContext ctx = context();
        ctx.free_shipping_threshold = inr("0.00");
        EXPECT_EQ(cart_totals(basket(), ctx).shipping, inr("49.00"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(cart_totals(one_book(), context()),
                  (CartSummary{inr("499.00"), inr("0.00"), std::nullopt, inr("49.00"), inr("0.00"),
                               inr("548.00")}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        CartContext ctx = context();
        ctx.region = "MH";
        EXPECT_EQ(cart_totals(one_book(), ctx),
                  (CartSummary{inr("499.00"), inr("0.00"), std::nullopt, inr("49.00"), inr("8.82"),
                               inr("556.82")}));
    }();
    [&] {
        SCOPED_TRACE("case 4");
        Cart cart = one_book();
        add_item(cart, "XYZ-99999", 1, inr("1.00"), kNow);
        EXPECT_THROW(cart_totals(cart, context()), CartError);
    }();
    [&] {
        SCOPED_TRACE("case 5");
        EXPECT_EQ(cart_totals(basket(), context()),
                  (CartSummary{inr("4097.00"), inr("0.00"), std::nullopt, inr("0.00"), inr("529.72"),
                               inr("4626.72")}));
    }();
}

}  // namespace
}  // namespace shopcore::cart
