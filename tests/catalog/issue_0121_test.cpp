// Issue 0121. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/catalog/catalog.hpp"

namespace shopcore::catalog {
namespace {

TEST(Issue0121, test_issue_0121) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(base_sku("APP-10001-BLU-M"), "APP-10001");
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(format_sku(SkuParts{"APP", "10001", "BLU", "M"}), "APP-10001-BLU-M");
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_FALSE(is_valid_sku("APP-10001-BLU-XM"));
        EXPECT_TRUE(is_valid_sku("APP-10001-BLU-XXL"));
    }();
    [&] {
        SCOPED_TRACE("case 4");
        EXPECT_TRUE(is_valid_sku("GRO-50001"));
        EXPECT_TRUE(is_valid_sku("APP-20001-RED-XL"));
    }();
    [&] {
        SCOPED_TRACE("case 5");
        EXPECT_EQ(parse_sku("APP-10001-BLU-M"), (SkuParts{"APP", "10001", "BLU", "M"}));
    }();
    [&] {
        SCOPED_TRACE("case 6");
        EXPECT_EQ(parse_sku("APP-10003-BLK-32").size, "32");
    }();
    [&] {
        SCOPED_TRACE("case 7");
        EXPECT_FALSE(is_valid_sku("GRO_50001"));
        EXPECT_FALSE(is_valid_sku(""));
    }();
}

}  // namespace
}  // namespace shopcore::catalog
