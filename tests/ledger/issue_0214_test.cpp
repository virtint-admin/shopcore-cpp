// Issue 0214. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/ledger/ledger.hpp"

namespace shopcore::ledger {
namespace {

TEST(Issue0214, test_issue_0214) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(parent_account("assets:bank"), "assets");
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(parent_account("assets:bank:hdfc"), "assets:bank");
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(parent_account("assets"), "");
    }();
}

}  // namespace
}  // namespace shopcore::ledger
