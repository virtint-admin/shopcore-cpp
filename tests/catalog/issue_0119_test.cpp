// Issue 0119. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/catalog/catalog.hpp"

namespace shopcore::catalog {
namespace {

TEST(Issue0119, test_issue_0119) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(size_rank(""), 0);
        EXPECT_THROW(size_rank("XXXL"), CatalogError);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(size_rank("XS"), 1);
        EXPECT_EQ(size_rank("M"), 3);
        EXPECT_EQ(size_rank("XXL"), 6);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(size_rank("32"), 132);
        EXPECT_EQ(size_rank("08"), 108);
    }();
}

}  // namespace
}  // namespace shopcore::catalog
