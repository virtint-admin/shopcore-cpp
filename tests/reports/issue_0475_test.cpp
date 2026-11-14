// Issue 0475. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "reports/report_fixtures.hpp"

namespace shopcore::reports {
namespace {

using testing::inr;

TEST(Issue0475, test_issue_0475) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(competition_rank({7, 7, 7}), (std::vector<int>{1, 1, 1}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(competition_rank({50, 30, 50, 10}), (std::vector<int>{1, 3, 1, 4}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_TRUE(competition_rank({}).empty());
    }();
}

}  // namespace
}  // namespace shopcore::reports
