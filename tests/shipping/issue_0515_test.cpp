// Issue 0515. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/shipping/shipping.hpp"
#include "support/fixtures.hpp"

namespace shopcore::shipping {
namespace {

using testing::date_of;
using testing::kIst;
using testing::ts;

TEST(Issue0515, test_issue_0515) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(business_days_between(date_of("2026-11-09"), date_of("2026-11-16"), {}), 6);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(business_days_between(date_of("2026-11-09"), date_of("2026-11-09"), {}), 0);
        EXPECT_EQ(business_days_between(date_of("2026-11-16"), date_of("2026-11-09"), {}), 0);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(business_days_between(date_of("2026-11-09"), date_of("2026-11-16"),
                                        {date_of("2026-11-11")}),
                  5);
    }();
}

}  // namespace
}  // namespace shopcore::shipping
