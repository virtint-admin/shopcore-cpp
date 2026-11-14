// Issue 0048. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0048, test_issue_0048) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(merge_carts(guest_cart(), customer_cart(), MergePolicy::Max, kT0).items[0].qty, 3);
        EXPECT_EQ(
            merge_carts(guest_cart(), customer_cart(), MergePolicy::PreferCustomer, kT0).items[0].qty,
            1);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        const Cart merged = merge_carts(guest_cart(), customer_cart(), MergePolicy::Sum, kT0 + 5);
        EXPECT_EQ(merged.id, "c-1");
        EXPECT_EQ(merged.customer_id, "cust-9");
        ASSERT_EQ(merged.items.size(), 2U);
        EXPECT_EQ(merged.items[0], (CartItem{"APP-10001-BLU-M", 4, inr("749.00"), kT0 - 1000}));
        EXPECT_EQ(merged.items[1].sku, "BOK-40001");
        EXPECT_EQ(merged.coupon_codes, (std::vector<std::string>{"STYLE15", "WELCOME10"}));
        EXPECT_EQ(merged.updated_at, kT0 + 5);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Cart usd_cart = new_cart("u", "", "USD", kT0);
        EXPECT_THROW(merge_carts(usd_cart, customer_cart(), MergePolicy::Sum, kT0),
                     money::CurrencyMismatch);
        Cart guest = guest_cart();
        guest.coupon_codes = {"STYLE15"};
        EXPECT_EQ(merge_carts(guest, customer_cart(), MergePolicy::Sum, kT0).coupon_codes,
                  (std::vector<std::string>{"STYLE15"}));
    }();
}

}  // namespace
}  // namespace shopcore::cart
