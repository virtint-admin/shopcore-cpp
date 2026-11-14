// Issue 0241. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "ledger/ledger_fixtures.hpp"

namespace shopcore::ledger {
namespace {

using money::Money;
using testing::inr;

const Date kDay1{2026, 11, 2};

TEST(Issue0241, test_issue_0241) {
    [&] {
        SCOPED_TRACE("case 1");
        Ledger ledger;
        post_entry(ledger, kDay1, "a", "r1",
                   {debit("assets:bank", inr("1.00")), credit("revenue:shipping", inr("0.00")),
                    credit("revenue:sales", inr("1.00"))});
        EXPECT_EQ(ledger.entries[0].postings.size(), 2U);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Ledger ledger;
        record_refund(ledger, kDay1, "pay_X", inr("100.00"), inr("0.00"));
        EXPECT_EQ(ledger.entries[0].postings.size(), 2U);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Ledger ledger;
        EXPECT_THROW(record_refund(ledger, kDay1, "pay_X", inr("0.00"), inr("0.00")), UnbalancedEntry);
    }();
    [&] {
        SCOPED_TRACE("case 4");
        EXPECT_EQ(sample::small().entries[2].postings.size(), 3U);
    }();
    [&] {
        SCOPED_TRACE("case 5");
        Ledger ledger;
        record_refund(ledger, kDay1, "pay_X", inr("100.00"), inr("18.00"));
        EXPECT_EQ(ledger.entries[0].postings,
                  (std::vector<Posting>{{"revenue:sales_returns", inr("100.00")},
                                        {"liabilities:gst_output", inr("18.00")},
                                        {"assets:receivable:gateway", inr("-118.00")}}));
        EXPECT_EQ(ledger.entries[0].memo, "Refund pay_X");
    }();
}

}  // namespace
}  // namespace shopcore::ledger
