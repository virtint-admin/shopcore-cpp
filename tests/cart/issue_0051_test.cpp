// Issue 0051. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0051, test_issue_0051) {
    [&] {
        SCOPED_TRACE("case 1");
        Cart cart = guest_cart();
        EXPECT_THROW(reprice_cart(cart, {{"BOK-40001", testing::usd("10.00")}}),
                     money::CurrencyMismatch);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Cart cart = guest_cart();
        const auto changed = reprice_cart(cart, {{"APP-10001-BLU-M", inr("699.00")},
                                                 {"BOK-40001", inr("899.00")},
                                                 {"ELE-30001", inr("1.00")}});
        EXPECT_EQ(changed, (std::vector<std::string>{"APP-10001-BLU-M"}));
        EXPECT_EQ(find_item(cart, "APP-10001-BLU-M")->unit_price, inr("699.00"));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Cart cart = guest_cart();
        EXPECT_TRUE(reprice_cart(cart, {}).empty());
        EXPECT_EQ(find_item(cart, "BOK-40001")->unit_price, inr("899.00"));
    }();
}

}  // namespace
}  // namespace shopcore::cart
