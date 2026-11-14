// Issue 0042. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/cart/cart.hpp"
#include "support/fixtures.hpp"

namespace shopcore::cart {
namespace {

using money::Money;
using testing::inr;

constexpr Timestamp kT0 = 1'794'200'000;

Cart cart_with(std::vector<std::pair<std::string, std::int64_t>> lines, const Money& unit) {
    Cart cart = new_cart("c", "", "INR", kT0);
    for (const auto& [sku, qty] : lines) {
        add_item(cart, sku, qty, unit, kT0);
    }
    return cart;
}

CartLimits small_limits() {
    CartLimits limits;
    limits.max_lines = 2;
    limits.max_qty_per_sku = 3;
    limits.max_total_units = 5;
    limits.max_value = inr("3000.00");
    return limits;
}

TEST(Issue0042, test_issue_0042) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_TRUE(
            can_add_item(cart_with({{"A", 2}}, inr("100.00")), "A", 1, inr("100.00"), small_limits()));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_TRUE(validate_cart(cart_with({{"A", 3}}, inr("999.00")), small_limits()).empty());
        CartLimits no_value = small_limits();
        no_value.max_value = inr("0.00");
        EXPECT_TRUE(validate_cart(cart_with({{"A", 3}}, inr("99999.00")), no_value).empty());
    }();
    [&] {
        SCOPED_TRACE("case 3");
        const Cart cart = cart_with({{"A", 2}}, inr("100.00"));
        EXPECT_FALSE(can_add_item(cart, "B", 0, inr("100.00"), small_limits()));
        EXPECT_FALSE(can_add_item(cart, "B", 1, testing::usd("1.00"), small_limits()));
    }();
    [&] {
        SCOPED_TRACE("case 4");
        const Cart cart = cart_with({{"A", 2}}, inr("100.00"));
        EXPECT_FALSE(can_add_item(cart, "A", 2, inr("100.00"), small_limits()));
        EXPECT_FALSE(can_add_item(cart, "B", 1, inr("2901.00"), small_limits()));
    }();
}

}  // namespace
}  // namespace shopcore::cart
