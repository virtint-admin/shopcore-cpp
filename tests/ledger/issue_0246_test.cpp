// Issue 0246. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "ledger/ledger_fixtures.hpp"

namespace shopcore::ledger {
namespace {

using money::Money;
using testing::inr;

const Date kDay1{2026, 11, 2};

TEST(Issue0246, test_issue_0246) {
    [&] {
        SCOPED_TRACE("case 1");
        const auto entries = entries_for_reference(sample::small(), "pay_N1a2b3c");
        ASSERT_EQ(entries.size(), 2U);
        EXPECT_EQ(entries[0].id, 1);
        EXPECT_EQ(entries[1].id, 2);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(entries_for_reference(sample::small(), "pay_N4d5e6f").size(), 1U);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_TRUE(entries_for_reference(sample::small(), "pay_X").empty());
    }();
}

}  // namespace
}  // namespace shopcore::ledger
