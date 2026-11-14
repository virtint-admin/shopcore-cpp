// Issue 0082. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0082, test_issue_0082) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(paginate(seed_products(), 4, 4).size(), 3U);
        EXPECT_TRUE(paginate(seed_products(), 5, 4).empty());
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(
            skus_of(paginate(seed_products(), 2, 4)),
            (std::vector<std::string>{"APP-20001-RED-XL", "ELE-30001", "ELE-30002", "ELE-30005"}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(paginate(seed_products(), 0, 10), CatalogError);
        EXPECT_THROW(paginate(seed_products(), 1, 0), CatalogError);
    }();
}

}  // namespace
}  // namespace shopcore::catalog
