// Issue 0577. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/tax/tax.hpp"
#include "support/fixtures.hpp"

namespace shopcore::tax {
namespace {

using testing::date_of;
using testing::seed_tax_rules;

const Date kToday{2026, 11, 9};

TEST(Issue0577, test_issue_0577) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_FALSE(is_valid_gstin("27AAPFU0939F1ZW"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_TRUE(is_valid_gstin("27AAPFU0939F1ZV"));
        EXPECT_TRUE(is_valid_gstin("29ABCDE1234F1ZW"));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_FALSE(is_valid_gstin("27AAPFU0939F1Z"));
        EXPECT_FALSE(is_valid_gstin("2AAAPFU0939F1ZV"));
        EXPECT_FALSE(is_valid_gstin("27AAPF10939F1ZV"));
        EXPECT_FALSE(is_valid_gstin("27AAPFU09A9F1ZV"));
        EXPECT_FALSE(is_valid_gstin("27AAPFU093991ZV"));
        EXPECT_FALSE(is_valid_gstin("27AAPFU0939F0ZV"));
        EXPECT_FALSE(is_valid_gstin("27AAPFU0939F1YV"));
    }();
}

}  // namespace
}  // namespace shopcore::tax
