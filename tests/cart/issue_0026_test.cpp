// Issue 0026. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0026, test_issue_0026) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(distinct_skus(sample_cart()), 2U);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Cart cart = sample_cart();
        cart.items.push_back(cart.items[0]);
        EXPECT_EQ(distinct_skus(cart), 2U);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(distinct_skus(new_cart("c", "", "INR", kT0)), 0U);
    }();
}

}  // namespace
}  // namespace shopcore::cart
