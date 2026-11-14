// Issue 0090. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0090, test_issue_0090) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(skus_of(search_products(seed_products(), "audio ELECTRONICS")),
                  (std::vector<std::string>{"ELE-30001", "ELE-30002"}));
        EXPECT_EQ(skus_of(search_products(seed_products(), "bok-40002")),
                  (std::vector<std::string>{"BOK-40002"}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(skus_of(search_products(seed_products(), "cotton")),
                  (std::vector<std::string>{"APP-10001-BLU-M", "APP-10001-BLU-L", "APP-10001-BLK-M"}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(tokenize_query("tea TEA, tea!"), (std::vector<std::string>{"tea"}));
    }();
    [&] {
        SCOPED_TRACE("case 4");
        EXPECT_EQ(tokenize_query("Cotton T-Shirt"), (std::vector<std::string>{"cotton", "t", "shirt"}));
    }();
    [&] {
        SCOPED_TRACE("case 5");
        EXPECT_TRUE(search_products(seed_products(), "bedsheet").empty());
        EXPECT_TRUE(search_products(seed_products(), "  ").empty());
    }();
}

}  // namespace
}  // namespace shopcore::catalog
