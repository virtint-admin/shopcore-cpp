// Issue 0109. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/catalog/catalog.hpp"

namespace shopcore::catalog {
namespace {

TEST(Issue0109, test_issue_0109) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(normalize_sku(" app_10001 blu--m "), "APP-10001-BLU-M");
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(normalize_sku("--ele-30001--"), "ELE-30001");
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(normalize_sku("   "), "");
    }();
}

}  // namespace
}  // namespace shopcore::catalog
