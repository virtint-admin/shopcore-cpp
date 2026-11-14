// Issue 0114. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/catalog/catalog.hpp"

namespace shopcore::catalog {
namespace {

TEST(Issue0114, test_issue_0114) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(ean13_check_digit("400638133393"), 1);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(ean13_check_digit("890123456789"), 0);
        EXPECT_EQ(ean13_check_digit("590123412345"), 7);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_TRUE(is_valid_ean13("4006381333931"));
        EXPECT_TRUE(is_valid_ean13("8901234567890"));
    }();
    [&] {
        SCOPED_TRACE("case 4");
        EXPECT_THROW(ean13_check_digit("12345"), CatalogError);
        EXPECT_THROW(ean13_check_digit("40063813339A"), CatalogError);
    }();
}

}  // namespace
}  // namespace shopcore::catalog
