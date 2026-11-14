// Issue 0575. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/tax/tax.hpp"
#include "support/fixtures.hpp"

namespace shopcore::tax {
namespace {

using testing::date_of;
using testing::seed_tax_rules;

const Date kToday{2026, 11, 9};

TEST(Issue0575, test_issue_0575) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(state_code_from_gstin("29ABCDE1234F1ZW"), "KA");
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(state_code_from_gstin("07AAACI1234A1ZE"), "DL");
        EXPECT_EQ(state_code_from_gstin("27AAPFU0939F1ZV"), "MH");
        EXPECT_EQ(state_code_from_gstin("33AAACR5055K1ZE"), "TN");
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(state_code_from_gstin("99ABCDE1234F1Z5"), TaxError);
        EXPECT_THROW(state_code_from_gstin("2"), TaxError);
    }();
}

}  // namespace
}  // namespace shopcore::tax
