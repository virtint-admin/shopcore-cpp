// cart: shopping cart contents, guest/customer cart merge, totals and limits.
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "shopcore/catalog/catalog.hpp"
#include "shopcore/core/core.hpp"
#include "shopcore/money/money.hpp"
#include "shopcore/pricing/pricing.hpp"
#include "shopcore/tax/tax.hpp"

namespace shopcore::cart {

class CartError : public DomainError {
public:
    using DomainError::DomainError;
};

inline constexpr std::size_t kMaxCouponCodes = 2;

struct CartItem {
    std::string sku;
    std::int64_t qty = 0;
    money::Money unit_price;
    Timestamp added_at = 0;

    bool operator==(const CartItem&) const = default;
};

struct Cart {
    std::string id;
    std::string customer_id;  // empty for a guest cart
    std::string currency = "INR";
    std::vector<CartItem> items;
    std::vector<std::string> coupon_codes;
    Timestamp created_at = 0;
    Timestamp updated_at = 0;

    bool operator==(const Cart&) const = default;
};

enum class MergePolicy {
    Sum,             // add quantities together
    Max,             // keep the larger quantity
    PreferCustomer,  // keep the customer's cart line untouched
};

struct CartAdjustment {
    std::string sku;
    std::int64_t old_qty = 0;
    std::int64_t new_qty = 0;

    bool operator==(const CartAdjustment&) const = default;
};

struct LineTotal {
    std::string sku;
    money::Money total;

    bool operator==(const LineTotal&) const = default;
};

struct CartContext {
    std::vector<catalog::Product> products;
    std::vector<tax::TaxRule> tax_rules;
    std::vector<pricing::Coupon> coupons;  // only codes applied to the cart are considered
    std::map<std::string, std::int64_t> customer_coupon_uses;
    std::string region = "KA";
    Date tax_date;
    Timestamp now = 0;
    money::Money shipping_fee;
    money::Money free_shipping_threshold;  // zero disables free shipping
};

struct CartSummary {
    money::Money subtotal;
    money::Money discount;
    std::optional<std::string> coupon_code;
    money::Money shipping;
    money::Money tax;
    money::Money total;

    bool operator==(const CartSummary&) const = default;
};

struct CartLimits {
    std::size_t max_lines = 50;
    std::int64_t max_qty_per_sku = 10;
    std::int64_t max_total_units = 100;
    money::Money max_value;

    bool operator==(const CartLimits&) const = default;
};

enum class LimitKind { QtyPerSku, Lines, TotalUnits, Value };

struct LimitViolation {
    LimitKind kind = LimitKind::QtyPerSku;
    std::string sku;  // only for QtyPerSku
    std::string detail;

    bool operator==(const LimitViolation&) const = default;
};

// ---- items.cpp --------------------------------------------------------------
Cart new_cart(std::string_view id, std::string_view customer_id, std::string_view currency,
              Timestamp now);
// Adds units; an existing line for the SKU is increased and re-priced.
void add_item(Cart& cart, std::string_view sku, std::int64_t qty, const money::Money& unit_price,
              Timestamp now);
bool remove_item(Cart& cart, std::string_view sku, Timestamp now);
// Sets the quantity of an existing line; 0 removes it.
void update_quantity(Cart& cart, std::string_view sku, std::int64_t qty, Timestamp now);
std::optional<CartItem> find_item(const Cart& cart, std::string_view sku);
// Total units in the cart.
std::int64_t item_count(const Cart& cart);
std::size_t distinct_skus(const Cart& cart);
void clear_cart(Cart& cart, Timestamp now);
// Adds a normalised coupon code; false if already applied. Throws over kMaxCouponCodes.
bool apply_coupon_code(Cart& cart, std::string_view code);
bool remove_coupon_code(Cart& cart, std::string_view code);

// ---- merge.cpp --------------------------------------------------------------
// Merges a guest cart into the customer's cart after login.
Cart merge_carts(const Cart& guest, const Cart& customer, MergePolicy policy, Timestamp now);
// Updates unit prices from the price list; returns SKUs whose price changed.
std::vector<std::string> reprice_cart(Cart& cart,
                                      const std::map<std::string, money::Money>& prices);
// Clamps quantities to available stock (missing SKU = 0 available, line removed).
std::vector<CartAdjustment> reconcile_with_stock(
    Cart& cart, const std::map<std::string, std::int64_t>& available);
bool is_abandoned(const Cart& cart, Timestamp now, std::int64_t idle_seconds);
// Stable identity of cart contents: "SKU:qty;SKU:qty" sorted by SKU.
std::string cart_fingerprint(const Cart& cart);

// ---- totals.cpp -------------------------------------------------------------
money::Money cart_subtotal(const Cart& cart);
std::int64_t cart_weight_grams(const Cart& cart, const std::vector<catalog::Product>& products);
// Subtotal, best applied coupon, shipping (free over the threshold) and regional tax.
CartSummary cart_totals(const Cart& cart, const CartContext& context);
// How much more the customer must add to get free shipping.
money::Money free_shipping_gap(const money::Money& subtotal, const money::Money& threshold);
std::vector<LineTotal> line_totals(const Cart& cart);

// ---- limits.cpp -------------------------------------------------------------
CartLimits default_cart_limits(std::string_view currency);
std::vector<LimitViolation> validate_cart(const Cart& cart, const CartLimits& limits);
bool can_add_item(const Cart& cart, std::string_view sku, std::int64_t qty,
                  const money::Money& unit_price, const CartLimits& limits);
std::int64_t max_addable_quantity(const Cart& cart, std::string_view sku,
                                  const money::Money& unit_price, const CartLimits& limits);
// Trims the cart until it satisfies the limits; returns the number of lines changed.
int clamp_to_limits(Cart& cart, const CartLimits& limits);

}  // namespace shopcore::cart
