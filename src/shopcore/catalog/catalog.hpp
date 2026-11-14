// catalog: products, variants, SKU parsing and validation, catalogue search.
//
// SKU format: CAT-STYLE[-COLOR[-SIZE]]
//   CAT   three upper-case letters (APP apparel, ELE electronics, BOK books, ...)
//   STYLE five digits
//   COLOR three upper-case letters (optional)
//   SIZE  XS, S, M, L, XL, XXL or a two-digit numeric size (optional, needs COLOR)
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "shopcore/core/core.hpp"
#include "shopcore/money/money.hpp"

namespace shopcore::catalog {

class CatalogError : public DomainError {
public:
    using DomainError::DomainError;
};

class InvalidSku : public CatalogError {
public:
    using CatalogError::CatalogError;
};

struct SkuParts {
    std::string category;
    std::string style;
    std::string color;  // empty when absent
    std::string size;   // empty when absent

    bool operator==(const SkuParts&) const = default;
};

struct Product {
    std::string sku;
    std::string name;
    std::string category;  // slash-separated path, e.g. "apparel/men/tshirts"
    money::Money price;
    money::Money cost;
    std::int64_t weight_grams = 0;
    std::int64_t length_mm = 0;
    std::int64_t width_mm = 0;
    std::int64_t height_mm = 0;
    std::string hsn_code;  // Harmonised System code used for GST
    bool active = true;
    Date launched;

    bool operator==(const Product&) const = default;
};

enum class SortKey { PriceAsc, PriceDesc, NameAsc, Newest };

struct PriceRange {
    money::Money min;
    money::Money max;

    bool operator==(const PriceRange&) const = default;
};

// ---- sku.cpp ----------------------------------------------------------------
SkuParts parse_sku(std::string_view sku);
std::string format_sku(const SkuParts& parts);
bool is_valid_sku(std::string_view sku);
// Trims, upper-cases, turns spaces/underscores into dashes and collapses repeats.
std::string normalize_sku(std::string_view raw);
// "APP-10001-BLU-M" -> "APP-10001".
std::string base_sku(std::string_view sku);
// GS1 check digit for the first 12 digits of an EAN-13.
int ean13_check_digit(std::string_view first_twelve);
bool is_valid_ean13(std::string_view barcode);
// Sort rank for a size: XS=1 ... XXL=6, numeric sizes rank 100 + value.
int size_rank(std::string_view size);

// ---- product.cpp ------------------------------------------------------------
// Parses data/products.csv format.
std::vector<Product> parse_product_csv(std::string_view csv);
// Returns human-readable problems; empty when the product is valid.
std::vector<std::string> validate_product(const Product& product);
std::optional<Product> find_product(const std::vector<Product>& products, std::string_view sku);
// Products whose category is `category` or a sub-category of it.
std::vector<Product> products_in_category(const std::vector<Product>& products,
                                          std::string_view category);
std::vector<Product> active_products(const std::vector<Product>& products);
// All variants sharing a base SKU, ordered by colour then size rank.
std::vector<Product> variants_of(const std::vector<Product>& products, std::string_view base);
// Package volume in cubic centimetres (rounded up).
std::int64_t volume_cm3(const Product& product);
std::vector<Product> sort_products(std::vector<Product> products, SortKey key);
PriceRange price_range(const std::vector<Product>& products);
std::vector<Product> apply_price_update(std::vector<Product> products, std::string_view sku,
                                        const money::Money& new_price);
std::vector<Product> deactivate_product(std::vector<Product> products, std::string_view sku);

// ---- search.cpp -------------------------------------------------------------
std::vector<std::string> tokenize_query(std::string_view query);
// Products (active only) matching every query token in name, category or SKU.
std::vector<Product> search_products(const std::vector<Product>& products, std::string_view query);
std::vector<Product> filter_by_price(const std::vector<Product>& products,
                                     const money::Money& min_price, const money::Money& max_price);
std::string slugify(std::string_view name);
// "apparel/men/tshirts" -> "Apparel > Men > Tshirts".
std::string category_breadcrumb(std::string_view category);
// 1-based page of results.
std::vector<Product> paginate(const std::vector<Product>& products, int page, int page_size);

}  // namespace shopcore::catalog
