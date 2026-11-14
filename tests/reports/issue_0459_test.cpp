// Issue 0459. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include <algorithm>

#include "reports/report_fixtures.hpp"

namespace shopcore::reports {
namespace {

using testing::inr;

TEST(Issue0459, test_issue_0459) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(format_percent_bp(-50), "-0.50%");
        EXPECT_EQ(format_percent_bp(5), "0.05%");
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(format_percent_bp(1234), "12.34%");
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(format_percent_bp(10000), "100.00%");
        EXPECT_EQ(format_percent_bp(0), "0.00%");
    }();
}

}  // namespace
}  // namespace shopcore::reports
