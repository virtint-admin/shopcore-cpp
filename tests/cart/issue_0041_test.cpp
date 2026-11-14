// Issue 0041. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0041, test_issue_0041) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(default_cart_limits("INR"), (CartLimits{50, 10, 100, inr("200000.00")}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(default_cart_limits("JPY").max_value, (Money{200000, "JPY"}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(default_cart_limits("XYZ"), money::UnknownCurrency);
    }();
}

}  // namespace
}  // namespace shopcore::cart
