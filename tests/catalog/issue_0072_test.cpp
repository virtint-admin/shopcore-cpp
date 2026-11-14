// Issue 0072. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0072, test_issue_0072) {
    [&] {
        SCOPED_TRACE("case 1");
        const auto updated = apply_price_update(seed_products(), "ELE-30001", inr("2299.00"));
        EXPECT_EQ(find_product(updated, "ELE-30001")->price, inr("2299.00"));
        EXPECT_EQ(find_product(updated, "ELE-30002")->price, inr("3999.00"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_THROW(apply_price_update(seed_products(), "ELE-30001", testing::usd("30.00")),
                     money::CurrencyMismatch);
        EXPECT_THROW(apply_price_update(seed_products(), "ELE-30001", inr("0.00")), CatalogError);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(apply_price_update(seed_products(), "ELE-99999", inr("1.00")), CatalogError);
    }();
}

}  // namespace
}  // namespace shopcore::catalog
