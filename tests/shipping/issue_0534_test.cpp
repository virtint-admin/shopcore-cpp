// Issue 0534. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0534, test_issue_0534) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(parcel_count(5000, 5000), 1);
        EXPECT_EQ(parcel_count(0, 5000), 0);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(parcel_count(10500, 5000), 3);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(parcel_count(100, 0), ShippingError);
        EXPECT_THROW(parcel_count(-1, 100), ShippingError);
    }();
}

}  // namespace
}  // namespace shopcore::shipping
