// Issue 0465. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "reports/report_fixtures.hpp"

namespace shopcore::reports {
namespace {

using testing::inr;

TEST(Issue0465, test_issue_0465) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(pareto_skus(sample::records(), 8000),
                  (std::vector<std::string>{"ELE-30001", "APP-20001-RED-XL"}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(pareto_skus(sample::records(), 10000).size(), 4U);
        EXPECT_EQ(pareto_skus(sample::records(), 1), (std::vector<std::string>{"ELE-30001"}));
        // ELE-30001 alone is 49.84% of revenue.
        EXPECT_EQ(pareto_skus(sample::records(), 4984), (std::vector<std::string>{"ELE-30001"}));
        EXPECT_EQ(pareto_skus(sample::records(), 4985).size(), 2U);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(pareto_skus(sample::records(), 0), ReportError);
        EXPECT_THROW(pareto_skus(sample::records(), 10001), ReportError);
        EXPECT_TRUE(pareto_skus({}, 8000).empty());
    }();
}

}  // namespace
}  // namespace shopcore::reports
