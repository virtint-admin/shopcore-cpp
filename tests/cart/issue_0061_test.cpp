// Issue 0061. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0061, test_issue_0061) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(cart_weight_grams(new_cart("c", "", "INR", kNow), {}), 0);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(cart_weight_grams(basket(), testing::seed_products()), 560);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Cart cart = basket();
        add_item(cart, "XYZ-99999", 1, inr("1.00"), kNow);
        EXPECT_THROW(cart_weight_grams(cart, testing::seed_products()), CartError);
    }();
}

}  // namespace
}  // namespace shopcore::cart
