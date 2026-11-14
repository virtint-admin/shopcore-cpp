// Issue 0180. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/inventory/inventory.hpp"
#include "support/fixtures.hpp"

namespace shopcore::inventory {
namespace {

using testing::inr;
using testing::seed_inventory;

TEST(Issue0180, test_issue_0180) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(low_stock_skus(seed_inventory(), 2), (std::vector<std::string>{"ELE-30005"}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_TRUE(low_stock_skus(seed_inventory(), 0).empty());
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(
            low_stock_skus(seed_inventory(), 5),
            (std::vector<std::string>{"APP-20001-RED-XL", "BOK-40002", "ELE-30005", "FUR-70001"}));
    }();
}

}  // namespace
}  // namespace shopcore::inventory
