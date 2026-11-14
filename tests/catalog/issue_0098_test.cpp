// Issue 0098. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0098, test_issue_0098) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(slugify("USB-C Charger 65W"), "usb-c-charger-65w");
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(slugify("Silk Saree, Kanjeevaram Red"), "silk-saree-kanjeevaram-red");
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(slugify("  --Hello--  "), "hello");
        EXPECT_EQ(slugify("!!!"), "");
    }();
}

}  // namespace
}  // namespace shopcore::catalog
