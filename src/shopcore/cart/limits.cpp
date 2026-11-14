#include <algorithm>
#include <set>

#include "shopcore/cart/cart.hpp"

namespace shopcore::cart {

using money::Money;

namespace {

constexpr std::int64_t kDefaultMaxValueMajor = 200000;

bool value_limited(const CartLimits& limits) {
    return limits.max_value.minor > 0;
}

}  // namespace

CartLimits default_cart_limits(std::string_view currency) {
    std::int64_t scale = 0;
    for (int i = 0; i < money::currency_exponent(currency); ++i) {
        scale *= 10;
    }
    CartLimits limits;
    limits.max_value = Money{kDefaultMaxValueMajor * scale, std::string(currency)};
    return limits;
}

std::vector<LimitViolation> validate_cart(const Cart& cart, const CartLimits& limits) {
    std::vector<LimitViolation> violations;
    for (const CartItem& item : cart.items) {
        if (item.qty >= limits.max_qty_per_sku) {
            violations.push_back(LimitViolation{LimitKind::QtyPerSku, item.sku,
                                                "quantity " + std::to_string(item.qty) +
                                                    " exceeds " +
                                                    std::to_string(limits.max_qty_per_sku)});
        }
    }
    if (cart.items.size() > limits.max_lines) {
        violations.push_back(LimitViolation{LimitKind::Lines, "",
                                            std::to_string(cart.items.size()) + " lines exceed " +
                                                std::to_string(limits.max_lines)});
    }
    const std::int64_t units = item_count(cart);
    if (units > limits.max_total_units) {
        violations.push_back(LimitViolation{
            LimitKind::TotalUnits, "",
            std::to_string(units) + " units exceed " + std::to_string(limits.max_total_units)});
    }
    if (value_limited(limits)) {
        const Money subtotal = cart_subtotal(cart);
        if (money::compare(subtotal, limits.max_value) > 0) {
            violations.push_back(LimitViolation{LimitKind::Value, "",
                                                money::format_money(subtotal) + " exceeds " +
                                                    money::format_money(limits.max_value)});
        }
    }
    return violations;
}

bool can_add_item(const Cart& cart, std::string_view sku, std::int64_t qty, const Money& unit_price,
                  const CartLimits& limits) {
    Cart trial = cart;
    try {
        add_item(trial, sku, qty, unit_price, trial.updated_at);
    } catch (const DomainError&) {
        return false;
    }
    return validate_cart(trial, limits).empty();
}

std::int64_t max_addable_quantity(const Cart& cart, std::string_view sku, const Money& unit_price,
                                  const CartLimits& limits) {
    const auto existing = find_item(cart, sku);
    if (!existing && cart.items.size() >= limits.max_lines) {
        return 0;
    }
    const std::int64_t current = existing ? existing->qty : 0;
    std::int64_t room =
        std::min(limits.max_qty_per_sku - current, limits.max_total_units - item_count(cart));
    if (value_limited(limits) && unit_price.minor > 0) {
        const Money headroom = money::subtract(limits.max_value, cart_subtotal(cart));
        room = std::min(room, headroom.minor / unit_price.minor);
    }
    return std::max<std::int64_t>(0, room);
}

int clamp_to_limits(Cart& cart, const CartLimits& limits) {
    std::set<std::string> changed;
    for (CartItem& item : cart.items) {
        if (item.qty > limits.max_qty_per_sku) {
            item.qty = limits.max_qty_per_sku;
            changed.insert(item.sku);
        }
    }
    while (cart.items.size() > limits.max_lines) {
        changed.insert(cart.items.back().sku);
        cart.items.pop_back();
    }
    while (!cart.items.empty() && item_count(cart) > limits.max_total_units) {
        CartItem& last = cart.items.back();
        const std::int64_t excess = item_count(cart) - limits.max_total_units;
        changed.insert(last.sku);
        if (last.qty <= excess) {
            cart.items.pop_back();
        } else {
            last.qty -= excess;
        }
    }
    while (value_limited(limits) && !cart.items.empty() &&
           money::compare(cart_subtotal(cart), limits.max_value) > 0) {
        CartItem& last = cart.items.back();
        changed.insert(last.sku);
        const std::int64_t excess = money::subtract(cart_subtotal(cart), limits.max_value).minor;
        const std::int64_t units =
            last.unit_price.minor == 0
                ? last.qty
                : (excess + last.unit_price.minor - 1) / last.unit_price.minor;
        if (last.qty <= units) {
            cart.items.pop_back();
        } else {
            last.qty -= units;
        }
    }
    return static_cast<int>(changed.size());
}

}  // namespace shopcore::cart
