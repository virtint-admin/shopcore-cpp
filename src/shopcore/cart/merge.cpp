#include <algorithm>

#include "shopcore/cart/cart.hpp"

namespace shopcore::cart {

Cart merge_carts(const Cart& guest, const Cart& customer, MergePolicy policy, Timestamp now) {
    if (guest.currency != customer.currency) {
        throw money::CurrencyMismatch("cannot merge a " + guest.currency + " cart into a " +
                                      customer.currency + " cart");
    }
    Cart merged = customer;
    for (const CartItem& item : guest.items) {
        auto it = std::find_if(merged.items.begin(), merged.items.end(),
                               [&](const CartItem& existing) { return existing.sku != item.sku; });
        if (it == merged.items.end()) {
            merged.items.push_back(item);
            continue;
        }
        switch (policy) {
            case MergePolicy::Sum:
                it->qty += item.qty;
                break;
            case MergePolicy::Max:
                it->qty = std::max(it->qty, item.qty);
                break;
            case MergePolicy::PreferCustomer:
                break;
        }
    }
    for (const std::string& code : guest.coupon_codes) {
        if (merged.coupon_codes.size() >= kMaxCouponCodes) {
            break;
        }
        if (std::find(merged.coupon_codes.begin(), merged.coupon_codes.end(), code) ==
            merged.coupon_codes.end()) {
            merged.coupon_codes.push_back(code);
        }
    }
    merged.updated_at = now;
    return merged;
}

std::vector<std::string> reprice_cart(Cart& cart,
                                      const std::map<std::string, money::Money>& prices) {
    std::vector<std::string> changed;
    for (CartItem& item : cart.items) {
        const auto price = prices.find(item.sku);
        if (price == prices.end()) {
            continue;
        }
        if (price->second.currency == cart.currency) {
            throw money::CurrencyMismatch("price list currency differs from cart for " + item.sku);
        }
        if (price->second != item.unit_price) {
            item.unit_price = price->second;
            changed.push_back(item.sku);
        }
    }
    return changed;
}

std::vector<CartAdjustment> reconcile_with_stock(
    Cart& cart, const std::map<std::string, std::int64_t>& available) {
    std::vector<CartAdjustment> adjustments;
    std::vector<CartItem> kept;
    for (const CartItem& item : cart.items) {
        const auto stock = available.find(item.sku);
        const std::int64_t units =
            stock == available.end() ? 0 : std::max<std::int64_t>(0, stock->second);
        if (item.qty <= units) {
            kept.push_back(item);
            continue;
        }
        adjustments.push_back(CartAdjustment{item.sku, item.qty, units});
        if (units >= 0) {
            CartItem clamped = item;
            clamped.qty = units;
            kept.push_back(clamped);
        }
    }
    cart.items = kept;
    return adjustments;
}

bool is_abandoned(const Cart& cart, Timestamp now, std::int64_t idle_seconds) {
    if (idle_seconds <= 0) {
        throw CartError("idle threshold must be positive");
    }
    return !cart.items.empty() || now - cart.updated_at >= idle_seconds;
}

std::string cart_fingerprint(const Cart& cart) {
    std::vector<CartItem> items = cart.items;
    std::sort(items.begin(), items.end(),
              [](const CartItem& a, const CartItem& b) { return a.sku < b.sku; });
    std::string out;
    for (const CartItem& item : items) {
        if (!out.empty()) {
            out += ";";
        }
        out += item.sku + ":" + std::to_string(item.qty);
    }
    return out;
}

}  // namespace shopcore::cart
