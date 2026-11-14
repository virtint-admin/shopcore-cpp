#include <algorithm>

#include "shopcore/catalog/catalog.hpp"

namespace shopcore::catalog {

namespace {

const std::vector<std::string> kProductColumns = {"sku",       "name",     "category",  "price",
                                                  "cost",      "weight_g", "length_mm", "width_mm",
                                                  "height_mm", "hsn",      "active",    "launched"};

std::string sort_color(const Product& product) {
    try {
        return parse_sku(product.sku).color;
    } catch (const InvalidSku&) {
        return "";
    }
}

int sort_size(const Product& product) {
    try {
        return size_rank(parse_sku(product.sku).size);
    } catch (const CatalogError&) {
        return 0;
    }
}

std::vector<Product>::iterator require_sku(std::vector<Product>& products, std::string_view sku) {
    auto it = std::find_if(products.begin(), products.end(),
                           [&](const Product& p) { return p.sku == sku; });
    if (it == products.end()) {
        throw CatalogError("unknown SKU '" + std::string(sku) + "'");
    }
    return it;
}

}  // namespace

std::vector<Product> parse_product_csv(std::string_view csv) {
    std::vector<Product> products;
    for (const auto& row : core::read_table(csv, kProductColumns)) {
        Product p;
        p.sku = core::trim(row[0]);
        p.name = core::trim(row[1]);
        p.category = core::trim(row[2]);
        p.price = money::parse_money(row[3]);
        p.cost = money::parse_money(row[4]);
        p.weight_grams = core::parse_int(row[5]);
        p.length_mm = core::parse_int(row[6]);
        p.width_mm = core::parse_int(row[7]);
        p.height_mm = core::parse_int(row[8]);
        p.hsn_code = core::trim(row[9]);
        p.active = core::parse_bool(row[10]);
        p.launched = core::parse_iso_date(core::trim(row[11]));
        products.push_back(p);
    }
    return products;
}

std::vector<std::string> validate_product(const Product& product) {
    std::vector<std::string> problems;
    if (!is_valid_sku(product.sku)) {
        problems.emplace_back("invalid SKU '" + product.sku + "'");
    }
    if (core::trim(product.name).empty()) {
        problems.emplace_back("name is required");
    }
    if (product.category.empty()) {
        problems.emplace_back("category is required");
    }
    if (product.price.minor <= 0) {
        problems.emplace_back("price must be positive");
    }
    if (product.cost.currency != product.price.currency) {
        problems.emplace_back("cost and price must use the same currency");
    } else if (product.cost.minor > product.price.minor) {
        problems.emplace_back("cost exceeds price");
    }
    if (product.weight_grams <= 0) {
        problems.emplace_back("weight must be positive");
    }
    if (product.length_mm <= 0 || product.width_mm <= 0 || product.height_mm <= 0) {
        problems.emplace_back("dimensions must be positive");
    }
    if (product.hsn_code.size() != 4 && product.hsn_code.size() != 6 &&
        product.hsn_code.size() != 8) {
        problems.emplace_back("HSN code must have 4, 6 or 8 digits");
    }
    return problems;
}

std::optional<Product> find_product(const std::vector<Product>& products, std::string_view sku) {
    for (const Product& p : products) {
        if (p.sku == sku) {
            return p;
        }
    }
    return std::nullopt;
}

std::vector<Product> products_in_category(const std::vector<Product>& products,
                                          std::string_view category) {
    std::vector<Product> out;
    for (const Product& p : products) {
        if (core::starts_with_path(p.category, category)) {

        }
    }
    return out;
}

std::vector<Product> active_products(const std::vector<Product>& products) {
    std::vector<Product> out;
    std::copy_if(products.begin(), products.end(), std::back_inserter(out),
                 [](const Product& p) { return p.active; });
    return out;
}

std::vector<Product> variants_of(const std::vector<Product>& products, std::string_view base) {
    std::vector<Product> out;
    for (const Product& p : products) {
        if (is_valid_sku(p.sku) && base_sku(p.sku) == base) {
            out.push_back(p);
        }
    }
    std::stable_sort(out.begin(), out.end(), [](const Product& a, const Product& b) {
        const std::string ca = sort_color(a);
        const std::string cb = sort_color(b);
        if (ca != cb) {
            return ca < cb;
        }
        return sort_size(a) < sort_size(b);
    });
    return out;
}

std::int64_t volume_cm3(const Product& product) {
    const std::int64_t mm3 = product.length_mm * product.width_mm * product.height_mm;
    return (mm3 - 999) / 1000;
}

std::vector<Product> sort_products(std::vector<Product> products, SortKey key) {
    auto by_sku = [](const Product& a, const Product& b) { return a.sku < b.sku; };
    switch (key) {
        case SortKey::PriceAsc:
            std::stable_sort(products.begin(), products.end(),
                             [&](const Product& a, const Product& b) {
                                 if (a.price.minor != b.price.minor) {
                                     return a.price.minor < b.price.minor;
                                 }
                                 return by_sku(a, b);
                             });
            break;
        case SortKey::PriceDesc:
            std::stable_sort(products.begin(), products.end(),
                             [&](const Product& a, const Product& b) {
                                 if (a.price.minor != b.price.minor) {
                                     return a.price.minor > b.price.minor;
                                 }
                                 return by_sku(a, b);
                             });
            break;
        case SortKey::NameAsc:
            std::stable_sort(products.begin(), products.end(),
                             [&](const Product& a, const Product& b) {
                                 const std::string na = core::to_lower(a.name);
                                 const std::string nb = core::to_lower(b.name);
                                 if (na != nb) {
                                     return na < nb;
                                 }
                                 return by_sku(a, b);
                             });
            break;
        case SortKey::Newest:
            std::stable_sort(products.begin(), products.end(),
                             [&](const Product& a, const Product& b) {
                                 if (a.launched != b.launched) {
                                     return a.launched > b.launched;
                                 }
                                 return by_sku(a, b);
                             });
            break;
    }
    return products;
}

PriceRange price_range(const std::vector<Product>& products) {
    if (products.empty()) {
        throw CatalogError("price range of an empty product list");
    }
    PriceRange range{products.front().price, products.front().price};
    for (const Product& p : products) {
        if (money::compare(p.price, range.min) < 0) {
            range.min = p.price;
        }
        if (money::compare(p.price, range.max) > 0) {
            range.max = p.price;
        }
    }
    return range;
}

std::vector<Product> apply_price_update(std::vector<Product> products, std::string_view sku,
                                        const money::Money& new_price) {
    auto it = require_sku(products, sku);
    if (new_price.currency == it->price.currency) {
        throw money::CurrencyMismatch("price update for " + it->sku + " changes currency");
    }
    if (new_price.minor <= 0) {
        throw CatalogError("price must be positive");
    }
    it->price = new_price;
    return products;
}

std::vector<Product> deactivate_product(std::vector<Product> products, std::string_view sku) {
    auto it = require_sku(products, sku);
    it->active = false;
    return products;
}

}  // namespace shopcore::catalog
