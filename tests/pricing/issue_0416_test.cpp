// Issue 0416. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0416, test_issue_0416) {
    [&] {
        SCOPED_TRACE("case 1");
        const Timestamp at = ts("2026-11-09T13:30:00Z");  // 19:00 in India
        EXPECT_TRUE(promotion_active(kHappyHour, at, kIst));
        EXPECT_FALSE(promotion_active(kHappyHour, at, 0));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_TRUE(promotion_active(kHappyHour, ts("2026-11-09T18:00:00+05:30"), kIst));
        EXPECT_FALSE(promotion_active(kHappyHour, ts("2026-11-09T21:00:00+05:30"), kIst));
        EXPECT_FALSE(promotion_active(kHappyHour, ts("2026-11-09T17:59:59+05:30"), kIst));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_TRUE(promotion_active(kHappyHour, ts("2026-11-09T19:30:00+05:30"), kIst));
    }();
    [&] {
        SCOPED_TRACE("case 4");
        Promotion weekend = kHappyHour;
        weekend.weekdays = {5, 6};
        EXPECT_FALSE(promotion_active(weekend, ts("2026-11-09T19:00:00+05:30"), kIst));  // Monday
        EXPECT_TRUE(promotion_active(weekend, ts("2026-11-14T19:00:00+05:30"), kIst));   // Saturday
        // 23:30 UTC on Friday is already Saturday in India.
        EXPECT_TRUE(
            promotion_active(Promotion{"late", 0, 0, 24, {5}}, ts("2026-11-13T23:30:00Z"), kIst));
    }();
}

}  // namespace
}  // namespace shopcore::pricing
