// Issue 0547. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/shipping/shipping.hpp"
#include "support/fixtures.hpp"

namespace shopcore::shipping {
namespace {

using testing::inr;
using testing::seed_rate_table;

const Parcel kBigBox{2000, 300, 250, 200};   // volumetric 3000 g
const Parcel kSmallBox{400, 100, 100, 100};  // volumetric 200 g

TEST(Issue0547, test_issue_0547) {
    [&] {
        SCOPED_TRACE("case 1");
        const RateTable table = seed_rate_table();
        ASSERT_EQ(table.bands.size(), 16U);
        EXPECT_EQ(table.bands.front(), (RateBand{"LOCAL", 500, inr("40.00")}));
        EXPECT_EQ(table.bands.back(), (RateBand{"REGIONAL", 5000, inr("180.00")}));
        EXPECT_EQ(table.extra_per_slab.at("METRO"), inr("30.00"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        const RateTable table = parse_rate_table_csv(
            "zone,max_weight_g,price\nlocal,1000,55.00 INR\nlocal,500,40.00 INR\n");
        EXPECT_EQ(table.bands,
                  (std::vector<RateBand>{{"LOCAL", 500, inr("40.00")}, {"LOCAL", 1000, inr("55.00")}}));
        EXPECT_TRUE(table.extra_per_slab.empty());
    }();
    [&] {
        SCOPED_TRACE("case 3");
        const std::string header = "zone,max_weight_g,price\n";
        EXPECT_THROW(parse_rate_table_csv(header + ",500,1.00 INR\n"), ShippingError);
        EXPECT_THROW(parse_rate_table_csv(header + "LOCAL,500,-1.00 INR\n"), ShippingError);
        EXPECT_THROW(parse_rate_table_csv(header + "LOCAL,+1000,1.00 INR\n"), ShippingError);
        EXPECT_THROW(parse_rate_table_csv(header + "LOCAL,0,1.00 INR\n"), ShippingError);
    }();
}

}  // namespace
}  // namespace shopcore::shipping
