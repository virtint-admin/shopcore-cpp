// Issue 0288. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0288, test_issue_0288) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(prorate_discount({line("A", 1, inr("1000.00")), line("B", 3, inr("1000.00"))},
                                   inr("100.00")),
                  (std::vector<Money>{inr("25.00"), inr("75.00")}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(prorate_discount({line("A", 1, inr("100.00")), line("B", 1, inr("100.00")),
                                    line("C", 1, inr("100.00"))},
                                   inr("1.00")),
                  (std::vector<Money>{inr("0.34"), inr("0.33"), inr("0.33")}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(prorate_discount({}, inr("1.00")), PricingError);
        EXPECT_THROW(prorate_discount({line("A", 1, inr("1.00"))}, usd("1.00")),
                     money::CurrencyMismatch);
    }();
}

}  // namespace
}  // namespace shopcore::pricing
