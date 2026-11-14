// Issue 0586. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/tax/tax.hpp"
#include "support/fixtures.hpp"

namespace shopcore::tax {
namespace {

using testing::date_of;
using testing::seed_tax_rules;

const Date kToday{2026, 11, 9};

TEST(Issue0586, test_issue_0586) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_TRUE(is_indian_state("KA"));
        EXPECT_TRUE(is_indian_state("DL"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(region_for_address(" in ", "ka"), "KA");
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(region_for_address("IN", "XX"), TaxError);
        EXPECT_THROW(region_for_address("US", ""), TaxError);
        EXPECT_THROW(region_for_address("GBR", ""), TaxError);
    }();
    [&] {
        SCOPED_TRACE("case 4");
        EXPECT_EQ(region_for_address("US", "ca"), "US-CA");
        EXPECT_EQ(region_for_address("gb", ""), "GB");
    }();
}

}  // namespace
}  // namespace shopcore::tax
