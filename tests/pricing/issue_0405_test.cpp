// Issue 0405. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/pricing/pricing.hpp"
#include "support/fixtures.hpp"

namespace shopcore::pricing {
namespace {

using money::Money;
using testing::inr;
using testing::usd;

LineItem line(std::string sku, std::int64_t qty, const Money& unit) {
    return LineItem{std::move(sku), "apparel/men/tshirts", qty, unit};
}

TEST(Issue0405, test_issue_0405) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(enforce_min_margin(inr("799.00"), inr("320.00"), 3000), inr("799.00"));
        EXPECT_EQ(enforce_min_margin(inr("0.00"), inr("320.00"), 2000), inr("400.00"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(enforce_min_margin(inr("400.00"), inr("320.00"), 3000), inr("457.15"));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(enforce_min_margin(inr("1.00"), inr("1.00"), 10000), PricingError);
        EXPECT_THROW(enforce_min_margin(inr("1.00"), inr("1.00"), -1), PricingError);
    }();
}

}  // namespace
}  // namespace shopcore::pricing
