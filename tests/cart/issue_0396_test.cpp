// Issue 0396. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/cart/cart.hpp"
#include "support/fixtures.hpp"

namespace shopcore::cart {
namespace {

using testing::inr;

constexpr Timestamp kT0 = 1'794'200'000;

Cart sample_cart() {
    Cart cart = new_cart("c-1", "cust-9", "INR", kT0);
    add_item(cart, "APP-10001-BLU-M", 2, inr("799.00"), kT0);
    add_item(cart, "ELE-30001", 1, inr("2499.00"), kT0 + 10);
    return cart;
}

TEST(Issue0396, test_issue_0396) {
    [&] {
        SCOPED_TRACE("case 1");
        Cart cart = sample_cart();
        EXPECT_THROW(apply_coupon_code(cart, "--"), CartError);
        apply_coupon_code(cart, "A1");
        apply_coupon_code(cart, "B2");
        EXPECT_THROW(apply_coupon_code(cart, "C3"), CartError);
        EXPECT_FALSE(apply_coupon_code(cart, "a1"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Cart cart = sample_cart();
        EXPECT_TRUE(apply_coupon_code(cart, " welcome-10 "));
        EXPECT_EQ(cart.coupon_codes, (std::vector<std::string>{"WELCOME10"}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Cart cart = sample_cart();
        apply_coupon_code(cart, "FLAT200");
        EXPECT_TRUE(remove_coupon_code(cart, "flat-200"));
        EXPECT_TRUE(cart.coupon_codes.empty());
    }();
    [&] {
        SCOPED_TRACE("case 4");
        Cart cart = sample_cart();
        apply_coupon_code(cart, "FLAT200");
        EXPECT_FALSE(apply_coupon_code(cart, "flat200"));
        EXPECT_EQ(cart.coupon_codes.size(), 1U);
    }();
}

}  // namespace
}  // namespace shopcore::cart
