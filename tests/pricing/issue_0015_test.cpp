// Issue 0015. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0015, test_issue_0015) {
    [&] {
        SCOPED_TRACE("case 1");
        const std::vector<LineItem> lines = {
            {"A", "c", 3, inr("100.00")}, {"B", "c", 1, inr("50.00")}, {"C", "c", 2, inr("30.00")}};
        EXPECT_EQ(cheapest_item_free(lines, 2), inr("180.00"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        const std::vector<LineItem> lines = {
            {"A", "c", 3, inr("100.00")}, {"B", "c", 1, inr("50.00")}, {"C", "c", 2, inr("30.00")}};
        EXPECT_EQ(cheapest_item_free(lines, 3), inr("130.00"));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(cheapest_item_free({}, 3), inr("0.00"));
        EXPECT_THROW(cheapest_item_free({tee(2)}, 1), PricingError);
        EXPECT_THROW(cheapest_item_free({tee(1), LineItem{"X", "c", 1, testing::usd("1.00")}}, 2),
                     money::CurrencyMismatch);
    }();
}

}  // namespace
}  // namespace shopcore::pricing
