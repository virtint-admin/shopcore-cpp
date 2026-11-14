// Issue 0282. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/catalog/catalog.hpp"
#include "support/fixtures.hpp"

namespace shopcore::catalog {
namespace {

using testing::inr;
using testing::seed_products;

std::vector<std::string> skus_of(const std::vector<Product>& products) {
    std::vector<std::string> skus;
    for (const Product& p : products) {
        skus.push_back(p.sku);
    }
    return skus;
}

TEST(Issue0282, test_issue_0282) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(skus_of(filter_by_price(seed_products(), inr("500.00"), inr("899.00"))),
                  (std::vector<std::string>{"APP-10001-BLU-M", "APP-10001-BLU-L", "APP-10001-BLK-M",
                                            "BOK-40001", "HOM-60001"}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(skus_of(filter_by_price(seed_products(), inr("349.00"), inr("349.00"))),
                  (std::vector<std::string>{"GRO-50001"}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(filter_by_price(seed_products(), inr("10.00"), inr("5.00")), CatalogError);
    }();
}

}  // namespace
}  // namespace shopcore::catalog
