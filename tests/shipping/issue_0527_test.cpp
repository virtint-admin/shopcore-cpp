// Issue 0527. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0527, test_issue_0527) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_FALSE(box_fits(Parcel{5001, 300, 200, 100}, Box{"M", 250, 350, 150, 5000}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(smallest_box(Parcel{2000, 300, 200, 100}, kBoxes)->name, "A-M");
        EXPECT_EQ(smallest_box(Parcel{6000, 300, 200, 100}, kBoxes)->name, "L");
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_FALSE(smallest_box(Parcel{100, 600, 100, 100}, kBoxes).has_value());
        EXPECT_FALSE(smallest_box(Parcel{100, 10, 10, 10}, {}).has_value());
    }();
    [&] {
        SCOPED_TRACE("case 4");
        EXPECT_EQ(smallest_box(Parcel{200, 180, 120, 90}, kBoxes)->name, "S");
    }();
}

}  // namespace
}  // namespace shopcore::shipping
