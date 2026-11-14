// Issue 0058. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/cart/cart.hpp"
#include "support/fixtures.hpp"

namespace shopcore::cart {
namespace {

using testing::inr;

constexpr Timestamp kT0 = 1'794'200'000;

Cart guest_cart() {
    Cart cart = new_cart("g-1", "", "INR", kT0);
    add_item(cart, "APP-10001-BLU-M", 3, inr("799.00"), kT0);
    add_item(cart, "BOK-40001", 1, inr("899.00"), kT0);
    cart.coupon_codes = {"WELCOME10", "FLAT200"};
    return cart;
}

Cart customer_cart() {
    Cart cart = new_cart("c-1", "cust-9", "INR", kT0 - 1000);
    add_item(cart, "APP-10001-BLU-M", 1, inr("749.00"), kT0 - 1000);
    cart.coupon_codes = {"STYLE15"};
    return cart;
}

TEST(Issue0058, test_issue_0058) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_FALSE(is_abandoned(new_cart("c", "", "INR", kT0), kT0 + 99999, 60));
        EXPECT_THROW(is_abandoned(guest_cart(), kT0, 0), CartError);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_FALSE(is_abandoned(guest_cart(), kT0 + 3599, 3600));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_TRUE(is_abandoned(guest_cart(), kT0 + 3600, 3600));
    }();
}

}  // namespace
}  // namespace shopcore::cart
