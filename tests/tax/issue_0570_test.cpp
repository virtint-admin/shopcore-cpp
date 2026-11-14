// Issue 0570. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/tax/tax.hpp"
#include "support/fixtures.hpp"

namespace shopcore::tax {
namespace {

using money::Money;
using testing::inr;

TEST(Issue0570, test_issue_0570) {
    [&] {
        SCOPED_TRACE("case 1");
        const std::vector<HsnLine> lines = {{"8518", inr("2499.00"), 1800},
                                            {"6109", inr("1598.00"), 500},
                                            {"6109", inr("799.00"), 500},
                                            {"6109", inr("1200.00"), 1200}};
        EXPECT_EQ(hsn_summary(lines),
                  (std::vector<HsnRow>{{"6109", 500, inr("2397.00"), inr("119.85")},
                                       {"6109", 1200, inr("1200.00"), inr("144.00")},
                                       {"8518", 1800, inr("2499.00"), inr("449.82")}}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        const std::vector<HsnLine> lines = {
            {"0902", inr("0.10"), 500}, {"0902", inr("0.10"), 500}, {"0902", inr("0.10"), 500}};
        EXPECT_EQ(hsn_summary(lines), (std::vector<HsnRow>{{"0902", 500, inr("0.30"), inr("0.02")}}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_TRUE(hsn_summary({}).empty());
    }();
}

}  // namespace
}  // namespace shopcore::tax
