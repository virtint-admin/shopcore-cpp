// Issue 0542. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/shipping/shipping.hpp"
#include "support/fixtures.hpp"

namespace shopcore::shipping {
namespace {

using testing::inr;
using testing::seed_rate_table;

const Parcel kBigBox{2000, 300, 250, 200};   // volumetric 3000 g
const Parcel kSmallBox{400, 100, 100, 100};  // volumetric 200 g

TEST(Issue0542, test_issue_0542) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(cod_fee(inr("1000.00")), inr("30.00"));
        EXPECT_EQ(cod_fee(inr("1500.00")), inr("30.00"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_THROW(cod_fee(testing::usd("100.00")), money::CurrencyMismatch);
        EXPECT_THROW(cod_fee(inr("0.00")), ShippingError);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(cod_fee(inr("2000.00")), inr("40.00"));
        EXPECT_EQ(cod_fee(inr("4999.99")), inr("100.00"));
    }();
}

}  // namespace
}  // namespace shopcore::shipping
