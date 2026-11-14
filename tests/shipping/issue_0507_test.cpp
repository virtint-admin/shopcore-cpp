// Issue 0507. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/shipping/shipping.hpp"
#include "support/fixtures.hpp"

namespace shopcore::shipping {
namespace {

using testing::date_of;
using testing::kIst;
using testing::ts;

TEST(Issue0507, test_issue_0507) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(format_delivery_window(DeliveryEstimate{date_of("2026-11-09"), date_of("2026-11-10"),
                                                          date_of("2026-11-12")}),
                  "Tue 10 Nov - Thu 12 Nov");
    }();
    [&] {
        SCOPED_TRACE("case 2");
        const Date d = date_of("2026-11-10");
        EXPECT_EQ(format_delivery_window(DeliveryEstimate{d, d, d}), "Tue 10 Nov");
        EXPECT_EQ(
            format_delivery_window(DeliveryEstimate{d, date_of("2026-12-31"), date_of("2027-01-02")}),
            "Thu 31 Dec - Sat 2 Jan");
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(format_delivery_window(DeliveryEstimate{
                         date_of("2026-11-09"), date_of("2026-11-12"), date_of("2026-11-10")}),
                     ShippingError);
    }();
}

}  // namespace
}  // namespace shopcore::shipping
