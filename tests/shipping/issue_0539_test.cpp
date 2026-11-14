// Issue 0539. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/shipping/shipping.hpp"
#include "support/fixtures.hpp"

namespace shopcore::shipping {
namespace {

using testing::inr;
using testing::seed_rate_table;

const Parcel kBigBox{2000, 300, 250, 200};   // volumetric 3000 g
const Parcel kSmallBox{400, 100, 100, 100};  // volumetric 200 g

TEST(Issue0539, test_issue_0539) {
    [&] {
        SCOPED_TRACE("case 1");
        const RateTable table = seed_rate_table();
        EXPECT_THROW(shipping_rate(table, "MARS", 100), ShippingError);
        EXPECT_THROW(shipping_rate(table, "LOCAL", 0), ShippingError);
        const RateTable capped = parse_rate_table_csv("zone,max_weight_g,price\nLOCAL,500,40.00 INR\n");
        EXPECT_THROW(shipping_rate(capped, "LOCAL", 501), ShippingError);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        const RateTable table = seed_rate_table();
        EXPECT_EQ(shipping_rate(table, "LOCAL", 5001), inr("160.00"));
        EXPECT_EQ(shipping_rate(table, "LOCAL", 6000), inr("180.00"));
        EXPECT_EQ(shipping_rate(table, "METRO", 7400), inr("370.00"));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        const RateTable table = seed_rate_table();
        EXPECT_EQ(shipping_rate(table, "LOCAL", 500), inr("40.00"));
        EXPECT_EQ(shipping_rate(table, "LOCAL", 501), inr("55.00"));
        EXPECT_EQ(shipping_rate(table, "NATIONAL", 5000), inr("280.00"));
    }();
}

}  // namespace
}  // namespace shopcore::shipping
