// Issue 0503. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/shipping/shipping.hpp"
#include "support/fixtures.hpp"

namespace shopcore::shipping {
namespace {

using testing::date_of;
using testing::kIst;
using testing::ts;

TEST(Issue0503, test_issue_0503) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_TRUE(is_late(date_of("2026-11-12"), ts("2026-11-12T20:00:00Z"), kIst));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(local_date(ts("2026-11-09T20:00:00Z"), kIst), date_of("2026-11-10"));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(local_date(ts("2026-11-09T02:00:00Z"), -300), date_of("2026-11-08"));
    }();
    [&] {
        SCOPED_TRACE("case 4");
        EXPECT_EQ(local_date(ts("2026-11-09T20:00:00Z"), 0), date_of("2026-11-09"));
    }();
}

}  // namespace
}  // namespace shopcore::shipping
