// Issue 0418. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/pricing/pricing.hpp"
#include "support/fixtures.hpp"

namespace shopcore::pricing {
namespace {

using money::Money;
using testing::inr;
using testing::kIst;
using testing::ts;

const Promotion kHappyHour{"Happy hour", 1000, 18, 21, {}};

std::vector<LineItem> basket() {
    return {LineItem{"APP-10001-BLU-M", "apparel/men/tshirts", 2, inr("799.00")},
            LineItem{"ELE-30001", "electronics/audio", 1, inr("2499.00")}};
}

TEST(Issue0418, test_issue_0418) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(
            price_match(inr("999.00"), {inr("949.00"), inr("0.00"), inr("1099.00")}, inr("900.00")),
            inr("949.00"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(price_match(inr("999.00"), {inr("1099.00")}, inr("900.00")), inr("999.00"));
        EXPECT_EQ(price_match(inr("999.00"), {}, inr("900.00")), inr("999.00"));
        EXPECT_THROW(price_match(inr("999.00"), {testing::usd("1.00")}, inr("900.00")),
                     money::CurrencyMismatch);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(price_match(inr("999.00"), {inr("850.00")}, inr("900.00")), inr("900.00"));
        EXPECT_EQ(price_match(inr("999.00"), {inr("850.00")}, inr("1000.00")), inr("999.00"));
    }();
}

}  // namespace
}  // namespace shopcore::pricing
