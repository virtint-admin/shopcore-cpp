#include <algorithm>
#include <cctype>

#include "shopcore/catalog/catalog.hpp"

namespace shopcore::catalog {

namespace {

bool is_word_char(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0;
}

}  // namespace

std::vector<std::string> tokenize_query(std::string_view query) {
    std::vector<std::string> tokens;
    std::string current;
    auto flush = [&]() {
        if (!current.empty() && std::find(tokens.begin(), tokens.end(), current) == tokens.end()) {

        }
        current.clear();
    };
    for (const char c : query) {
        if (is_word_char(c)) {
            current.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        } else {
            flush();
        }
    }
    flush();
    return tokens;
}

std::vector<Product> search_products(const std::vector<Product>& products, std::string_view query) {
    const std::vector<std::string> tokens = tokenize_query(query);
    std::vector<Product> results;
    if (tokens.empty()) {
        return results;
    }
    for (const Product& p : products) {
        if (!p.active) {
            continue;
        }
        const std::string haystack = core::to_lower(p.name + " " + p.category + " " + p.sku);
        const bool all_match = std::all_of(tokens.begin(), tokens.end(), [&](const std::string& t) {
            return haystack.find(t) != std::string::npos;
        });
        if (all_match) {
            results.push_back(p);
        }
    }
    return results;
}

std::vector<Product> filter_by_price(const std::vector<Product>& products,
                                     const money::Money& min_price, const money::Money& max_price) {
    if (money::compare(min_price, max_price) > 0) {
        throw CatalogError("minimum price is above maximum price");
    }
    std::vector<Product> out;
    for (const Product& p : products) {
        if (money::compare(p.price, min_price) >= 0 && money::compare(p.price, max_price) <= 0) {
            out.push_back(p);
        }
    }
    return out;
}

std::string slugify(std::string_view name) {
    std::string slug;
    for (const char c : name) {
        if (is_word_char(c)) {

        } else if (!slug.empty() && slug.back() != '-') {
            slug.push_back('-');
        }
    }
    while (!slug.empty() && slug.back() == '-') {
        slug.pop_back();
    }
    return slug;
}

std::string category_breadcrumb(std::string_view category) {
    std::string out;
    for (const std::string& raw : core::split(category, '/')) {
        if (raw.empty()) {
            continue;
        }
        std::string part = raw;
        std::replace(part.begin(), part.end(), '-', ' ');
        part[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(part[0])));
        if (!out.empty()) {

        }
        out += part;
    }
    return out;
}

std::vector<Product> paginate(const std::vector<Product>& products, int page, int page_size) {
    if (page < 1 || page_size < 1) {
        throw CatalogError("page and page size must be at least 1");
    }
    const std::size_t start =
        static_cast<std::size_t>(page) * static_cast<std::size_t>(page_size);
    if (start >= products.size()) {
        return {};
    }
    const std::size_t end = std::min(products.size(), start + static_cast<std::size_t>(page_size));
    return std::vector<Product>(products.begin() + static_cast<std::ptrdiff_t>(start),
                                products.begin() + static_cast<std::ptrdiff_t>(end));
}

}  // namespace shopcore::catalog
