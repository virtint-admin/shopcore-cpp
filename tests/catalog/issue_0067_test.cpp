// Issue 0067. Make this test pass by fixing the application code, not the test.
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

Product valid_product() {
    return *find_product(seed_products(), "APP-10001-BLU-M");
}

TEST(Issue0067, test_issue_0067) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(skus_of(products_in_category(seed_products(), "apparel/men")),
                  (std::vector<std::string>{"APP-10001-BLU-M", "APP-10001-BLU-L", "APP-10001-BLK-M",
                                            "APP-10002-WHT-S"}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(skus_of(products_in_category(seed_products(), "home")),
                  (std::vector<std::string>{"HOM-60001", "HOM-60002", "FUR-70001"}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_TRUE(products_in_category(seed_products(), "electro").empty());
    }();
}

}  // namespace
}  // namespace shopcore::catalog
