// Issue 0101. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0101, test_issue_0101) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(category_breadcrumb("home/kitchen-tools"), "Home > Kitchen tools");
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(category_breadcrumb("/books//tech/"), "Books > Tech");
        EXPECT_EQ(category_breadcrumb(""), "");
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(category_breadcrumb("apparel/men/tshirts"), "Apparel > Men > Tshirts");
    }();
}

}  // namespace
}  // namespace shopcore::catalog
