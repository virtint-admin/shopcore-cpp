// Issue 0526. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0526, test_issue_0526) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_TRUE(is_oversize(Parcel{100, 1501, 10, 10}));
        EXPECT_TRUE(is_oversize(Parcel{100, 1400, 500, 400}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_TRUE(is_oversize(Parcel{30001, 100, 100, 100}));
        EXPECT_FALSE(is_oversize(Parcel{30000, 100, 100, 100}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_FALSE(is_oversize(Parcel{5000, 1000, 300, 300}));
        EXPECT_FALSE(is_oversize(Parcel{100, 1500, 375, 375}));
    }();
}

}  // namespace
}  // namespace shopcore::shipping
