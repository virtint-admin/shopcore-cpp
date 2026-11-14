// Issue 0450. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/pricing/pricing.hpp"
#include "support/fixtures.hpp"

namespace shopcore::pricing {
namespace {

using testing::inr;

std::vector<PriceTier> tiers() {
    return {{1, inr("100.00")}, {10, inr("90.00")}, {50, inr("80.00")}};
}

TEST(Issue0450, test_issue_0450) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(next_tier_savings(tiers(), 49), (NextTier{1, inr("10.00")}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(next_tier_savings(tiers(), 8), (NextTier{2, inr("10.00")}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_FALSE(next_tier_savings(tiers(), 50).has_value());
    }();
}

}  // namespace
}  // namespace shopcore::pricing
