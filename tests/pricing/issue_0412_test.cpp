// Issue 0412. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0412, test_issue_0412) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(charm_price(inr("812.00")), inr("819.00"));
        EXPECT_EQ(charm_price(inr("1000.00")), inr("999.00"));
        EXPECT_EQ(charm_price(inr("1001.00")), inr("1009.00"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(charm_price(Money{1234, "JPY"}), (Money{1239, "JPY"}));
        EXPECT_THROW(charm_price(inr("0.00")), PricingError);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(charm_price(inr("45.50")), inr("45.99"));
        EXPECT_EQ(charm_price(inr("45.00")), inr("44.99"));
    }();
}

}  // namespace
}  // namespace shopcore::pricing
