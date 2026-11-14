// Issue 0038. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0038, test_issue_0038) {
    [&] {
        SCOPED_TRACE("case 1");
        Cart cart = cart_with({{"A", 1}}, inr("1.00"));
        EXPECT_EQ(clamp_to_limits(cart, small_limits()), 0);
        EXPECT_EQ(item_count(cart), 1);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Cart cart = cart_with({{"A", 5}, {"B", 3}, {"C", 1}}, inr("100.00"));
        CartLimits limits = small_limits();
        limits.max_value = inr("0.00");
        EXPECT_EQ(clamp_to_limits(cart, limits), 3);
        EXPECT_EQ(cart_fingerprint(cart), "A:3;B:2");
    }();
    [&] {
        SCOPED_TRACE("case 3");
        CartLimits limits = default_cart_limits("INR");
        limits.max_value = inr("2000.00");
        EXPECT_EQ(
            max_addable_quantity(cart_with({{"A", 2}}, inr("799.00")), "A", inr("799.00"), limits), 0);
        limits.max_value = inr("5000.00");
        EXPECT_EQ(
            max_addable_quantity(cart_with({{"A", 2}}, inr("799.00")), "B", inr("799.00"), limits), 4);
        EXPECT_EQ(max_addable_quantity(cart_with({{"A", 3}, {"B", 1}}, inr("1.00")), "C", inr("0.00"),
                                       CartLimits{5, 3, 5, inr("0.00")}),
                  1);
    }();
    [&] {
        SCOPED_TRACE("case 4");
        const Cart cart = cart_with({{"A", 2}, {"B", 2}, {"C", 2}}, inr("1.00"));
        EXPECT_EQ(validate_cart(cart, small_limits()),
                  (std::vector<LimitViolation>{{LimitKind::Lines, "", "3 lines exceed 2"},
                                               {LimitKind::TotalUnits, "", "6 units exceed 5"}}));
    }();
    [&] {
        SCOPED_TRACE("case 5");
        Cart cart = cart_with({{"A", 2}, {"B", 2}}, inr("100.00"));
        EXPECT_EQ(clamp_to_limits(cart, CartLimits{50, 10, 100, inr("250.00")}), 1);
        EXPECT_EQ(cart_fingerprint(cart), "A:2");
        Cart partial = cart_with({{"A", 2}, {"B", 3}}, inr("100.00"));
        EXPECT_EQ(clamp_to_limits(partial, CartLimits{50, 10, 100, inr("350.00")}), 1);
        EXPECT_EQ(cart_fingerprint(partial), "A:2;B:1");
    }();
}

}  // namespace
}  // namespace shopcore::cart
