// Issue 0266. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "ledger/ledger_fixtures.hpp"

namespace shopcore::ledger {
namespace {

using money::Money;
using testing::inr;
using testing::seed_settlements;

TEST(Issue0266, test_issue_0266) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(open_receivables(sample::small(), Date{2026, 11, 3}),
                  (std::vector<std::pair<std::string, Money>>{{"pay_N4d5e6f", inr("2948.82")}}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(reconcile_settlements(sample::unsettled(), seed_settlements()),
                  (Reconciliation{{"pay_N1a2b3c", "pay_N4d5e6f", "pay_N7g8h9i"},
                                  {"pay_Nm4n5o6"},
                                  {"pay_UNSETTLED"},
                                  {{"pay_Nj1k2l3", inr("1100.00"), inr("1000.00")}}}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Ledger ledger = sample::small();
        const Reconciliation result = reconcile_settlements(ledger, {seed_settlements()[0]});
        EXPECT_EQ(result.missing_in_ledger, (std::vector<std::string>{"pay_N1a2b3c"}));
        EXPECT_TRUE(result.unsettled.empty());
    }();
    [&] {
        SCOPED_TRACE("case 4");
        const Reconciliation result = reconcile_settlements(sample::unsettled(), {});
        EXPECT_TRUE(result.matched.empty());
        EXPECT_EQ(result.unsettled.size(), 6U);
    }();
}

}  // namespace
}  // namespace shopcore::ledger
