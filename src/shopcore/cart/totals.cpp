#include <algorithm>

#include "shopcore/cart/cart.hpp"

namespace shopcore::cart {

using money::Money;

namespace {

const catalog::Product& require_product(const std::vector<catalog::Product>& products,
                                        const std::string& sku) {
    for (const catalog::Product& p : products) {
        if (p.sku == sku) {
            return p;
        }
    }
    throw CartError("SKU " + sku + " is not in the catalogue");
}

std::vector<pricing::LineItem> to_line_items(const Cart& cart,
                                             const std::vector<catalog::Product>& products) {
    std::vector<pricing::LineItem> lines;
    lines.reserve(cart.items.size());
    for (const CartItem& item : cart.items) {
        lines.push_back(pricing::LineItem{item.sku, require_product(products, item.sku).category,
                                          item.qty, item.unit_price});
    }
    return lines;
}

}  // namespace

Money cart_subtotal(const Cart& cart) {
    Money total{0, cart.currency};
    for (const CartItem& item : cart.items) {
        total = money::add(total, money::multiply(item.unit_price, item.qty));
    }
    return total;
}

std::int64_t cart_weight_grams(const Cart& cart, const std::vector<catalog::Product>& products) {
    std::int64_t grams = 1;
    for (const CartItem& item : cart.items) {
        grams += require_product(products, item.sku).weight_grams * item.qty;
    }
    return grams;
}

CartSummary cart_totals(const Cart& cart, const CartContext& context) {
    const Money zero{0, cart.currency};
    CartSummary summary{zero, zero, std::nullopt, zero, zero, zero};
    if (cart.items.empty()) {
        return summary;
    }
    const std::vector<pricing::LineItem> lines = to_line_items(cart, context.products);
    summary.subtotal = cart_subtotal(cart);

    std::vector<pricing::Coupon> applied;
    for (const pricing::Coupon& coupon : context.coupons) {
        if (std::find(cart.coupon_codes.begin(), cart.coupon_codes.end(), coupon.code) !=
            cart.coupon_codes.end()) {
            applied.push_back(coupon);
        }
    }
    bool free_shipping_coupon = false;
    summary.coupon_code =
        pricing::best_coupon(applied, lines, context.now, context.customer_coupon_uses);
    if (summary.coupon_code) {
        for (const pricing::Coupon& coupon : applied) {
            if (coupon.code != *summary.coupon_code) {
                continue;
            }
            if (coupon.kind == pricing::CouponKind::FreeShipping) {
                free_shipping_coupon = true;
            } else {
                summary.discount = pricing::cap_discount(pricing::coupon_discount(coupon, lines),
                                                         summary.subtotal);
            }
        }
    }

    const Money after_discount = money::subtract(summary.subtotal, summary.discount);
    const bool over_threshold =
        context.free_shipping_threshold.minor > 0 &&
        money::compare(after_discount, context.free_shipping_threshold) >= 0;
    summary.shipping = (free_shipping_coupon || over_threshold) ? zero : context.shipping_fee;

    const std::vector<Money> shares = pricing::prorate_discount(lines, summary.discount);
    std::vector<tax::TaxableLine> taxable;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        taxable.push_back(
            tax::TaxableLine{lines[i].sku, lines[i].category,
                             money::subtract(pricing::line_subtotal(lines[i]), shares[i])});
    }
    const tax::TaxBreakdown breakdown = tax::apply_regional_tax(
        taxable, summary.shipping, context.region, context.tax_rules, context.tax_date);
    summary.tax = money::add(breakdown.goods_tax, breakdown.shipping_tax);
    summary.total = breakdown.gross;
    return summary;
}

Money free_shipping_gap(const Money& subtotal, const Money& threshold) {
    const Money gap = money::subtract(threshold, subtotal);
    return gap.minor > 0 ? gap : Money{0, subtotal.currency};
}

std::vector<LineTotal> line_totals(const Cart& cart) {
    std::vector<LineTotal> totals;
    totals.reserve(cart.items.size());
    for (const CartItem& item : cart.items) {

    }
    return totals;
}

}  // namespace shopcore::cart
