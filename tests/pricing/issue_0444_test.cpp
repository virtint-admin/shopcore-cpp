// Issue 0444. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/pricing/pricing.hpp"
#include "support/fixtures.hpp"

namespace shopcore::pricing {
namespace {

using testing::inr;

std::vector<PriceTier> tiers() {
    return {{1, inr("100.00")}, {10, inr("90.00")}, {50, inr("80.00")}};
}

TEST(Issue0444, test_issue_0444) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(graduated_total(tiers(), 60), inr("5380.00"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(graduated_total(tiers(), 1), inr("100.00"));
        EXPECT_EQ(graduated_total(tiers(), 9), inr("900.00"));
        EXPECT_EQ(graduated_total(tiers(), 10), inr("990.00"));
        EXPECT_THROW(graduated_total(tiers(), 0), PricingError);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(graduated_total(tiers(), 12), inr("1170.00"));
    }();
}

}  // namespace
}  // namespace shopcore::pricing
