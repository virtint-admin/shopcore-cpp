// Issue 0440. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/pricing/pricing.hpp"
#include "support/fixtures.hpp"

namespace shopcore::pricing {
namespace {

using testing::inr;

std::vector<PriceTier> tiers() {
    return {{1, inr("100.00")}, {10, inr("90.00")}, {50, inr("80.00")}};
}

TEST(Issue0440, test_issue_0440) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(tier_for_quantity(tiers(), 10), (PriceTier{10, inr("90.00")}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(tiered_unit_price(tiers(), 50), inr("80.00"));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(volume_discount_bp(tiers(), 1), 0);
        EXPECT_EQ(volume_discount_bp(tiers(), 10), 1000);
    }();
    [&] {
        SCOPED_TRACE("case 4");
        EXPECT_EQ(volume_discount_bp({{1, inr("0.00")}}, 5), 0);
        EXPECT_EQ(volume_discount_bp({{1, inr("3.00")}, {2, inr("2.00")}}, 2), 3333);
    }();
    [&] {
        SCOPED_TRACE("case 5");
        EXPECT_EQ(volume_discount_bp(tiers(), 50), 2000);
    }();
}

}  // namespace
}  // namespace shopcore::pricing
