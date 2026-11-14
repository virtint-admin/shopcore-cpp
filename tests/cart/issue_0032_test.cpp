// Issue 0032. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0032, test_issue_0032) {
    [&] {
        SCOPED_TRACE("case 1");
        Cart cart = new_cart("c", "", "INR", kT0);
        add_item(cart, "A", 1, inr("1.00"), kT0);
        EXPECT_TRUE(remove_item(cart, "A", kT0));
        EXPECT_TRUE(cart.items.empty());
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Cart cart = sample_cart();
        EXPECT_TRUE(remove_item(cart, "APP-10001-BLU-M", kT0 + 30));
        EXPECT_EQ(cart.items.size(), 1U);
        EXPECT_EQ(cart.updated_at, kT0 + 30);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Cart cart = sample_cart();
        EXPECT_FALSE(remove_item(cart, "BOK-40001", kT0 + 30));
        EXPECT_EQ(cart.updated_at, kT0 + 10);
    }();
}

}  // namespace
}  // namespace shopcore::cart
