// Issue 0369. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/pricing/pricing.hpp"
#include "support/fixtures.hpp"

namespace shopcore::pricing {
namespace {

using money::Money;
using testing::inr;

const Bundle kTeePair{"TEE-PAIR", {{"APP-10001-BLU-M", 2}}, Money{139900, "INR"}};
const Bundle kTech{"TECH", {{"ELE-30001", 1}, {"ELE-30005", 1}}, Money{399900, "INR"}};

LineItem tee(std::int64_t qty) {
    return LineItem{"APP-10001-BLU-M", "apparel/men/tshirts", qty, inr("799.00")};
}
LineItem earbuds(std::int64_t qty) {
    return LineItem{"ELE-30001", "electronics/audio", qty, inr("2499.00")};
}
LineItem charger(std::int64_t qty) {
    return LineItem{"ELE-30005", "electronics/accessories", qty, inr("1899.00")};
}

TEST(Issue0369, test_issue_0369) {
    [&] {
        SCOPED_TRACE("case 1");
        const LineItem tea{"GRO-50001", "grocery/beverages", 2, inr("349.00")};
        EXPECT_EQ(buy_x_get_y({tea}, "GRO-50001", 2, 1), inr("0.00"));
        EXPECT_EQ(buy_x_get_y({tea}, "ELE-30001", 1, 1), inr("0.00"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        const LineItem tea{"GRO-50001", "grocery/beverages", 7, inr("349.00")};
        EXPECT_EQ(buy_x_get_y({tea}, "GRO-50001", 2, 1), inr("698.00"));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(buy_x_get_y({tee(3)}, "APP-10001-BLU-M", 0, 1), PricingError);
        EXPECT_THROW(buy_x_get_y({tee(3)}, "APP-10001-BLU-M", 1, 0), PricingError);
    }();
}

}  // namespace
}  // namespace shopcore::pricing
