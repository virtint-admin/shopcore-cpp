// Issue 0470. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "reports/report_fixtures.hpp"

namespace shopcore::reports {
namespace {

using testing::inr;

TEST(Issue0470, test_issue_0470) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(slow_movers({"ELE-30001", "APP-20001-RED-XL", "BOK-40001", "GRO-50001", "BOK-40001"},
                              sample::records(), 1),
                  (std::vector<std::string>{"APP-20001-RED-XL", "BOK-40001"}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_TRUE(slow_movers({"ELE-30001"}, sample::records(), 2).empty());
        EXPECT_EQ(slow_movers({"ELE-30001"}, sample::records(), 3),
                  (std::vector<std::string>{"ELE-30001"}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(slow_movers({"ELE-30001", "BOK-40001"}, sample::records(), 0),
                  (std::vector<std::string>{"BOK-40001"}));
    }();
}

}  // namespace
}  // namespace shopcore::reports
