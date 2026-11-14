// Issue 0514. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/shipping/shipping.hpp"
#include "support/fixtures.hpp"

namespace shopcore::shipping {
namespace {

using testing::date_of;
using testing::kIst;
using testing::ts;

TEST(Issue0514, test_issue_0514) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(add_business_days(date_of("2026-11-15"), 0, {}), date_of("2026-11-15"));
        EXPECT_THROW(add_business_days(date_of("2026-11-15"), -1, {}), ShippingError);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        const Date today = date_of("2026-11-09");
        EXPECT_EQ(estimate_delivery(ts("2026-11-09T10:00:00+05:30"), kIst, "LOCAL",
                                    ServiceLevel::SameDay, {}, 14),
                  (DeliveryEstimate{today, today, today}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(
            estimate_delivery(ts("2026-11-09T10:00:00+05:30"), kIst, "METRO", ServiceLevel::Express,
                              {date_of("2026-11-11")}, 14),
            (DeliveryEstimate{date_of("2026-11-09"), date_of("2026-11-10"), date_of("2026-11-12")}));
    }();
    [&] {
        SCOPED_TRACE("case 4");
        EXPECT_EQ(
            estimate_delivery(ts("2026-11-09T10:00:00+05:30"), kIst, "NATIONAL", ServiceLevel::Standard,
                              {}, 14),
            (DeliveryEstimate{date_of("2026-11-09"), date_of("2026-11-13"), date_of("2026-11-17")}));
    }();
}

}  // namespace
}  // namespace shopcore::shipping
