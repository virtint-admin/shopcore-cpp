// Issue 0035. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0035, test_issue_0035) {
    [&] {
        SCOPED_TRACE("case 1");
        Cart cart = sample_cart();
        update_quantity(cart, "APP-10001-BLU-M", 7, kT0 + 40);
        EXPECT_EQ(find_item(cart, "APP-10001-BLU-M")->qty, 7);
        EXPECT_EQ(cart.updated_at, kT0 + 40);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Cart cart = sample_cart();
        update_quantity(cart, "ELE-30001", 0, kT0 + 40);
        EXPECT_FALSE(find_item(cart, "ELE-30001").has_value());
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Cart cart = sample_cart();
        EXPECT_THROW(update_quantity(cart, "ELE-30001", -1, kT0), CartError);
        EXPECT_THROW(update_quantity(cart, "BOK-40001", 1, kT0), CartError);
    }();
}

}  // namespace
}  // namespace shopcore::cart
