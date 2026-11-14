// Issue 0071. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0071, test_issue_0071) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(volume_cm3(valid_product()), 1500);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Product p = valid_product();
        p.length_mm = 101;
        p.width_mm = 1;
        p.height_mm = 1;
        EXPECT_EQ(volume_cm3(p), 1);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(volume_cm3(*find_product(seed_products(), "ELE-30002")), 2400);
    }();
}

}  // namespace
}  // namespace shopcore::catalog
