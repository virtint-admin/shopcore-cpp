// Issue 0437. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/pricing/pricing.hpp"
#include "support/fixtures.hpp"

namespace shopcore::pricing {
namespace {

using testing::inr;

std::vector<PriceTier> tiers() {
    return {{1, inr("100.00")}, {10, inr("90.00")}, {50, inr("80.00")}};
}

TEST(Issue0437, test_issue_0437) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_THROW(parse_tiers("1-100", "INR"), PricingError);
        EXPECT_THROW(parse_tiers("2:100", "INR"), PricingError);
        EXPECT_THROW(parse_tiers("1:100,10:110", "INR"), PricingError);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_FALSE(validate_tiers({{1, inr("10.00")}, {5, inr("11.00")}}));
        EXPECT_FALSE(validate_tiers({{1, inr("-1.00")}}));
        EXPECT_FALSE(validate_tiers({{1, inr("10.00")}, {5, testing::usd("1.00")}}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(parse_tiers("1:100.00,10:90.00,50:80.00", "INR"), tiers());
    }();
    [&] {
        SCOPED_TRACE("case 4");
        EXPECT_EQ(parse_tiers(" 1 : 5.5 , 3:5", "USD"),
                  (std::vector<PriceTier>{{1, testing::usd("5.50")}, {3, testing::usd("5.00")}}));
    }();
}

}  // namespace
}  // namespace shopcore::pricing
