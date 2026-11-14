// Issue 0521. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/shipping/shipping.hpp"

namespace shopcore::shipping {
namespace {

const std::vector<Box> kBoxes = {
    {"L", 500, 400, 300, 20000},
    {"S", 200, 150, 100, 1000},
    {"M", 350, 250, 150, 5000},
    {"A-M", 150, 350, 250, 5000},
};

TEST(Issue0521, test_issue_0521) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(pack_by_weight({{"A", 3000, 2}, {"B", 1500, 3}, {"C", 500, 1}}, 5000),
                  (std::vector<std::int64_t>{5000, 4500, 1500}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(pack_by_weight({{"A", 220, 2}, {"B", 120, 1}}, 5000),
                  (std::vector<std::int64_t>{560}));
        EXPECT_TRUE(pack_by_weight({}, 5000).empty());
        EXPECT_TRUE(pack_by_weight({{"A", 100, 0}}, 5000).empty());
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(pack_by_weight({{"A", 6000, 1}}, 5000), ShippingError);
        EXPECT_THROW(pack_by_weight({{"A", 0, 1}}, 5000), ShippingError);
        EXPECT_THROW(pack_by_weight({{"A", 10, -1}}, 5000), ShippingError);
        EXPECT_THROW(pack_by_weight({{"A", 10, 1}}, 0), ShippingError);
    }();
}

}  // namespace
}  // namespace shopcore::shipping
