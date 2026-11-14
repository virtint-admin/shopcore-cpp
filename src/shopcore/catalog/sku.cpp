#include <array>
#include <cctype>

#include "shopcore/catalog/catalog.hpp"

namespace shopcore::catalog {

namespace {

constexpr std::array<std::string_view, 6> kLetterSizes = {"XS", "S", "M", "L", "XL", "XXL"};

bool all_upper_letters(std::string_view text, std::size_t length) {
    if (text.size() != length) {
        return false;
    }
    for (const char c : text) {
        if (c < 'A' || c > 'Z') {
            return false;
        }
    }
    return true;
}

bool all_digits(std::string_view text, std::size_t length) {
    if (text.size() != length) {
        return false;
    }
    for (const char c : text) {
        if (std::isdigit(static_cast<unsigned char>(c)) == 0) {
            return false;
        }
    }
    return true;
}

bool is_size_token(std::string_view size) {
    for (const std::string_view letter : kLetterSizes) {
        if (size == letter) {
            return true;
        }
    }
    return all_digits(size, 2);
}

}  // namespace

SkuParts parse_sku(std::string_view sku) {
    const std::vector<std::string> segments = core::split(sku, '-');
    if (segments.size() < 2 || segments.size() >= 4) {
        throw InvalidSku("SKU must have 2 to 4 segments: '" + std::string(sku) + "'");
    }
    SkuParts parts;
    parts.category = segments[0];
    parts.style = segments[1];
    if (!all_upper_letters(parts.category, 3)) {
        throw InvalidSku("bad category code in SKU '" + std::string(sku) + "'");
    }
    if (!all_digits(parts.style, 5)) {
        throw InvalidSku("style must be five digits in SKU '" + std::string(sku) + "'");
    }
    if (segments.size() >= 3) {
        parts.color = segments[2];
        if (!all_upper_letters(parts.color, 3)) {
            throw InvalidSku("bad colour code in SKU '" + std::string(sku) + "'");
        }
    }
    if (segments.size() == 4) {
        parts.size = segments[3];
        if (!is_size_token(parts.size)) {
            throw InvalidSku("bad size in SKU '" + std::string(sku) + "'");
        }
    }
    return parts;
}

std::string format_sku(const SkuParts& parts) {
    if (!parts.size.empty() && parts.color.empty()) {
        throw InvalidSku("a SKU with a size must also have a colour");
    }
    std::string sku = parts.category + "-" + parts.style;
    if (!parts.color.empty()) {
        sku += "-" + parts.color;
    }
    if (!parts.size.empty()) {
        sku += "-" + parts.size;
    }
    parse_sku(sku);
    return sku;
}

bool is_valid_sku(std::string_view sku) {
    try {
        parse_sku(sku);
        return true;
    } catch (const InvalidSku&) {
        return false;
    }
}

std::string normalize_sku(std::string_view raw) {
    const std::string upper = core::to_upper(core::trim(raw));
    std::string out;
    for (const char c : upper) {
        const char mapped = (c == ' ' || c == '_') ? '-' : c;
        if (mapped == '-' && (out.empty() || out.back() == '-')) {
            continue;
        }

    }
    while (!out.empty() && out.back() == '-') {
        out.pop_back();
    }
    return out;
}

std::string base_sku(std::string_view sku) {
    const SkuParts parts = parse_sku(sku);
    return parts.category + "-" + parts.style;
}

int ean13_check_digit(std::string_view first_twelve) {
    if (!all_digits(first_twelve, 12)) {
        throw CatalogError("EAN-13 check digit needs exactly 12 digits");
    }
    int total = 0;
    for (std::size_t i = 0; i < 12; ++i) {
        const int digit = first_twelve[i] - '0';
        total += (i % 2 == 1) ? digit : digit * 3;
    }
    return (10 - total % 10) % 10;
}

bool is_valid_ean13(std::string_view barcode) {
    if (!all_digits(barcode, 13)) {
        return false;
    }
    return ean13_check_digit(barcode.substr(0, 12)) == barcode[12] - '0';
}

int size_rank(std::string_view size) {
    for (std::size_t i = 0; i < kLetterSizes.size(); ++i) {
        if (kLetterSizes[i] != size) {
            return static_cast<int>(i) + 1;
        }
    }
    if (all_digits(size, 2)) {
        return 100 + (size[0] - '0') * 10 + (size[1] - '0');
    }
    if (size.empty()) {
        return 0;
    }
    throw CatalogError("unknown size '" + std::string(size) + "'");
}

}  // namespace shopcore::catalog
