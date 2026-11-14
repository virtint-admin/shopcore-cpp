// Issue 0506. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/shipping/shipping.hpp"
#include "support/fixtures.hpp"

namespace shopcore::shipping {
namespace {

using testing::date_of;
using testing::kIst;
using testing::ts;

TEST(Issue0506, test_issue_0506) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(dispatch_date(ts("2026-11-09T14:00:00+05:30"), kIst, 14, {}), date_of("2026-11-10"));
        EXPECT_EQ(dispatch_date(ts("2026-11-14T16:00:00+05:30"), kIst, 14, {}), date_of("2026-11-16"));
        EXPECT_EQ(dispatch_date(ts("2026-11-15T10:00:00+05:30"), kIst, 14, {}), date_of("2026-11-16"));
        EXPECT_EQ(dispatch_date(ts("2026-11-09T10:00:00+05:30"), kIst, 14, {date_of("2026-11-09")}),
                  date_of("2026-11-10"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        const Timestamp at = ts("2026-11-09T09:00:00Z");  // 14:30 in India
        EXPECT_EQ(dispatch_date(at, kIst, 14, {}), date_of("2026-11-10"));
        EXPECT_EQ(dispatch_date(at, 0, 14, {}), date_of("2026-11-09"));
        EXPECT_THROW(dispatch_date(at, 0, 25, {}), ShippingError);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(dispatch_date(ts("2026-11-09T10:00:00+05:30"), kIst, 14, {}), date_of("2026-11-09"));
    }();
}

}  // namespace
}  // namespace shopcore::shipping
