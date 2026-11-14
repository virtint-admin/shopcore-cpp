// Issue 0461. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include <algorithm>

#include "reports/report_fixtures.hpp"

namespace shopcore::reports {
namespace {

using testing::inr;

TEST(Issue0461, test_issue_0461) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(report_title("Daily sales", Date{2026, 11, 1}, Date{2026, 11, 7}),
                  "Daily sales 2026-11-01..2026-11-07");
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(report_title("Top products", Date{2026, 11, 9}, Date{2026, 11, 9}),
                  "Top products 2026-11-09");
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(report_title("x", Date{2026, 11, 9}, Date{2026, 11, 8}), ReportError);
    }();
}

}  // namespace
}  // namespace shopcore::reports
