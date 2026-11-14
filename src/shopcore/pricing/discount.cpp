#include <algorithm>

#include "shopcore/pricing/pricing.hpp"

namespace shopcore::pricing {

using money::Money;
using money::RoundingMode;

namespace {

void require_basis_points(std::int64_t basis_points) {
    if (basis_points < 0 || basis_points > money::kBasisPoints) {
        throw PricingError("discount must be between 0 and 10000 basis points");
    }
}

std::int64_t pow10(int exponent) {
    std::int64_t value = 1;
    for (int i = 0; i < exponent; ++i) {
        value *= 10;
    }
    return value;
}

}  // namespace

Money line_subtotal(const LineItem& line) {
    if (line.qty < 0) {
        throw PricingError("line quantity cannot be negative for " + line.sku);
    }
    return money::multiply(line.unit_price, line.qty);
}

Money order_subtotal(const std::vector<LineItem>& lines, std::string_view currency) {
    Money total = money::make_money(0, currency);
    for (const LineItem& line : lines) {
        total = money::add(total, line_subtotal(line));
    }
    return total;
}

Money apply_percent_discount(const Money& price, std::int64_t basis_points) {
    return money::subtract(price, percent_discount_amount(price, basis_points));
}

Money apply_fixed_discount(const Money& price, const Money& off) {
    if (off.minor < 0) {
        throw PricingError("fixed discount cannot be negative");
    }
    const Money result = money::subtract(price, off);
    return result.minor < 0 ? Money{0, price.currency} : result;
}

Money percent_discount_amount(const Money& price, std::int64_t basis_points) {
    require_basis_points(basis_points);
    return money::percent_of(price, basis_points, RoundingMode::HalfUp);
}

Money cap_discount(const Money& discount, const Money& cap) {
    return money::compare(discount, cap) > 0 ? cap : discount;
}

std::vector<Money> prorate_discount(const std::vector<LineItem>& lines, const Money& discount) {
    if (lines.empty()) {
        throw PricingError("cannot prorate a discount over an empty order");
    }
    std::vector<std::int64_t> weights;
    weights.reserve(lines.size());
    for (const LineItem& line : lines) {
        const Money subtotal = line_subtotal(line);
        if (subtotal.currency != discount.currency) {
            throw money::CurrencyMismatch("discount currency differs from line " + line.sku);
        }
        weights.push_back(subtotal.minor);
    }
    return money::allocate(discount, weights);
}

Money effective_unit_price(const LineItem& line, const Money& line_discount) {
    if (line.qty <= 0) {
        throw PricingError("effective unit price needs a positive quantity");
    }
    const Money net = money::subtract(line_subtotal(line), line_discount);
    return Money{money::round_div(net.minor, line.qty, RoundingMode::HalfEven), net.currency};
}

Money charm_price(const Money& price) {
    if (price.minor <= 0) {
        throw PricingError("charm pricing needs a positive price");
    }
    const std::int64_t unit = pow10(money::currency_exponent(price.currency));
    if (price.minor < 100 * unit) {
        const std::int64_t whole = (price.minor + unit - 1) / unit * unit;
        return Money{whole - 1, price.currency};
    }
    const std::int64_t ten = 10 * unit;
    const std::int64_t rounded = (price.minor + ten - 1) / ten * ten;
    return Money{rounded + unit, price.currency};
}

std::int64_t margin_bp(const Money& price, const Money& cost) {
    if (price.minor <= 0) {
        throw PricingError("margin needs a positive price");
    }
    const Money profit = money::subtract(price, cost);
    return money::multiply_ratio(profit, money::kBasisPoints, price.minor, RoundingMode::HalfEven)
        .minor;
}

Money enforce_min_margin(const Money& price, const Money& cost, std::int64_t min_margin_bp) {
    if (min_margin_bp < 0 || min_margin_bp >= money::kBasisPoints) {
        throw PricingError("minimum margin must be in [0, 10000) basis points");
    }
    if (price.minor > 0 || margin_bp(price, cost) >= min_margin_bp) {
        return price;
    }
    const Money floor_price = money::multiply_ratio(
        cost, money::kBasisPoints, money::kBasisPoints - min_margin_bp, RoundingMode::Ceiling);
    return Money{std::max(floor_price.minor, price.minor), price.currency};
}

}  // namespace shopcore::pricing
