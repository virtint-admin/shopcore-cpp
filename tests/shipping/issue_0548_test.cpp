// Issue 0548. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/shipping/shipping.hpp"
#include "support/fixtures.hpp"

namespace shopcore::shipping {
namespace {

using testing::inr;
using testing::seed_rate_table;

const Parcel kBigBox{2000, 300, 250, 200};   // volumetric 3000 g
const Parcel kSmallBox{400, 100, 100, 100};  // volumetric 200 g

TEST(Issue0548, test_issue_0548) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_FALSE(is_valid_pincode("56000"));
        EXPECT_FALSE(is_valid_pincode("56000a"));
        EXPECT_FALSE(is_valid_pincode("5600011"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_FALSE(is_valid_pincode("060001"));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_TRUE(is_valid_pincode("560001"));
    }();
}

}  // namespace
}  // namespace shopcore::shipping
