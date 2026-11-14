// Issue 0244. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "ledger/ledger_fixtures.hpp"

namespace shopcore::ledger {
namespace {

using money::Money;
using testing::inr;

const Date kDay1{2026, 11, 2};

TEST(Issue0244, test_issue_0244) {
    [&] {
        SCOPED_TRACE("case 1");
        Ledger ledger = sample::small();
        reverse_entry(ledger, 3, Date{2026, 11, 3});
        EXPECT_EQ(account_balance(ledger, "revenue:sales", Date{2026, 11, 30}), inr("-1400.00"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Ledger ledger = sample::small();
        const std::int64_t id = reverse_entry(ledger, 3, Date{2026, 11, 4});
        EXPECT_EQ(id, 4);
        const JournalEntry reversal = *find_entry(ledger, id);
        EXPECT_EQ(reversal.memo, "Reversal of #3");
        EXPECT_EQ(reversal.reference, "pay_N4d5e6f");
        EXPECT_EQ(reversal.postings[0], (Posting{"assets:receivable:gateway", inr("-2948.82")}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Ledger ledger = sample::small();
        EXPECT_THROW(reverse_entry(ledger, 99, Date{2026, 11, 4}), LedgerError);
        EXPECT_THROW(reverse_entry(ledger, 3, Date{2026, 11, 2}), LedgerError);
    }();
}

}  // namespace
}  // namespace shopcore::ledger
