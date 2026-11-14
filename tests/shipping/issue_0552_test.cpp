// Issue 0552. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/shipping/shipping.hpp"
#include "support/fixtures.hpp"

namespace shopcore::shipping {
namespace {

using testing::inr;
using testing::seed_rate_table;

const Parcel kBigBox{2000, 300, 250, 200};   // volumetric 3000 g
const Parcel kSmallBox{400, 100, 100, 100};  // volumetric 200 g

TEST(Issue0552, test_issue_0552) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(quote_shipment(seed_rate_table(), "560001", "560034", {kBigBox, kSmallBox},
                                 ServiceLevel::Express),
                  inr("270.00"));
        EXPECT_EQ(
            quote_shipment(seed_rate_table(), "560001", "781001", {kSmallBox}, ServiceLevel::Standard),
            inr("75.00"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(zone_for_route("560001", "562110"), "REGIONAL");
        EXPECT_EQ(zone_for_route("560001", "781001"), "NATIONAL");
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(quote_shipment(seed_rate_table(), "560001", "560034", {}, ServiceLevel::Standard),
                     ShippingError);
        EXPECT_THROW(
            quote_shipment(seed_rate_table(), "560001", "400001", {kSmallBox}, ServiceLevel::SameDay),
            ShippingError);
    }();
    [&] {
        SCOPED_TRACE("case 4");
        EXPECT_EQ(quote_shipment(seed_rate_table(), "560001", "560034", {kBigBox, kSmallBox},
                                 ServiceLevel::Standard),
                  inr("180.00"));
    }();
}

}  // namespace
}  // namespace shopcore::shipping
