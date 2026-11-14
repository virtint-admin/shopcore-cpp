// Issue 0378. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0378, test_issue_0378) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(bundle_savings(Bundle{"DEAR", {{"APP-10001-BLU-M", 1}}, inr("900.00")}, {tee(1)}),
                  inr("0.00"));
        EXPECT_THROW(
            bundle_savings(Bundle{"USD", {{"APP-10001-BLU-M", 1}}, testing::usd("9.00")}, {tee(1)}),
            money::CurrencyMismatch);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(bundle_savings(kTeePair, {tee(5)}), inr("398.00"));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(bundle_savings(kTech, {earbuds(1)}), inr("0.00"));
    }();
}

}  // namespace
}  // namespace shopcore::pricing
