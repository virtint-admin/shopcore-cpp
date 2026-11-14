// Issue 0056. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0056, test_issue_0056) {
    [&] {
        SCOPED_TRACE("case 1");
        Cart cart = guest_cart();
        const auto adjustments = reconcile_with_stock(cart, {{"APP-10001-BLU-M", 2}});
        EXPECT_EQ(adjustments,
                  (std::vector<CartAdjustment>{{"APP-10001-BLU-M", 3, 2}, {"BOK-40001", 1, 0}}));
        ASSERT_EQ(cart.items.size(), 1U);
        EXPECT_EQ(cart.items[0].qty, 2);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Cart cart = guest_cart();
        const auto adjustments =
            reconcile_with_stock(cart, {{"APP-10001-BLU-M", -4}, {"BOK-40001", 1}});
        EXPECT_EQ(adjustments, (std::vector<CartAdjustment>{{"APP-10001-BLU-M", 3, 0}}));
        EXPECT_EQ(cart.items.size(), 1U);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Cart cart = guest_cart();
        EXPECT_TRUE(reconcile_with_stock(cart, {{"APP-10001-BLU-M", 3}, {"BOK-40001", 10}}).empty());
        EXPECT_EQ(cart.items.size(), 2U);
    }();
}

}  // namespace
}  // namespace shopcore::cart
