// Issue 0492. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "orders/order_fixtures.hpp"
#include "reports/report_fixtures.hpp"

namespace shopcore::reports {
namespace {

using money::Money;
using testing::inr;
using testing::kIst;

const Date kNov1{2026, 11, 1};
const Date kNov5{2026, 11, 5};

TEST(Issue0492, test_issue_0492) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_TRUE(moving_average({}, 3).empty());
        EXPECT_EQ(moving_average({5, 7}, 1), (std::vector<std::int64_t>{5, 7}));
        EXPECT_THROW(moving_average({1}, 0), ReportError);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(moving_average({10, 20, 30, 40}, 3), (std::vector<std::int64_t>{10, 15, 20, 30}));
        EXPECT_EQ(moving_average({1, 2, 2}, 2), (std::vector<std::int64_t>{1, 2, 2}));
        EXPECT_EQ(moving_average({1, 4}, 2), (std::vector<std::int64_t>{1, 2}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(moving_average({10, 20, 30, 40}, 2), (std::vector<std::int64_t>{10, 15, 25, 35}));
    }();
}

}  // namespace
}  // namespace shopcore::reports
