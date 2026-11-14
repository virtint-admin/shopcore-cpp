// pricing: discounts, coupons, tiered (volume) pricing, bundles and promotions.
//
// All prices here are pre-tax list prices. Tax is applied afterwards by the tax module,
// always on the discounted amount.
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "shopcore/core/core.hpp"
#include "shopcore/money/money.hpp"

namespace shopcore::pricing {

class PricingError : public DomainError {
public:
    using DomainError::DomainError;
};

struct LineItem {
    std::string sku;
    std::string category;  // product category path, used by category-scoped coupons
    std::int64_t qty = 0;
    money::Money unit_price;

    bool operator==(const LineItem&) const = default;
};

enum class CouponKind { Percent, Fixed, FreeShipping };

struct Coupon {
    std::string code;
    CouponKind kind = CouponKind::Percent;
    std::int64_t percent_bp = 0;               // Percent coupons
    money::Money amount;                       // Fixed coupons
    money::Money min_subtotal;                 // on the eligible subtotal
    std::optional<money::Money> max_discount;  // cap for Percent coupons
    Timestamp valid_from = 0;                  // inclusive
    Timestamp valid_until = 0;                 // exclusive
    std::int64_t max_uses = 0;                 // 0 = unlimited
    std::int64_t uses = 0;
    std::vector<std::string> categories;  // empty = whole order
    bool once_per_customer = false;

    bool operator==(const Coupon&) const = default;
};

enum class CouponStatus {
    Ok,
    NotYetValid,
    Expired,
    UsageExhausted,
    AlreadyUsed,
    NotApplicable,
    BelowMinimum,
};

struct PriceTier {
    std::int64_t min_qty = 1;
    money::Money unit_price;

    bool operator==(const PriceTier&) const = default;
};

struct NextTier {
    std::int64_t units_needed = 0;
    money::Money unit_savings;

    bool operator==(const NextTier&) const = default;
};

struct BundleComponent {
    std::string sku;
    std::int64_t qty = 1;

    bool operator==(const BundleComponent&) const = default;
};

struct Bundle {
    std::string id;
    std::vector<BundleComponent> components;
    money::Money price;  // price of one complete bundle

    bool operator==(const Bundle&) const = default;
};

struct AppliedBundle {
    std::string id;
    std::int64_t count = 0;

    bool operator==(const AppliedBundle&) const = default;
};

struct BundleResult {
    std::vector<AppliedBundle> applied;
    money::Money savings;
    std::vector<LineItem> remaining;  // units not consumed by any bundle

    bool operator==(const BundleResult&) const = default;
};

// Time-of-day promotion ("happy hour"), evaluated in the store's local time.
struct Promotion {
    std::string name;
    std::int64_t percent_bp = 0;
    int start_hour = 0;         // inclusive, 0-23
    int end_hour = 24;          // exclusive, 1-24
    std::vector<int> weekdays;  // 0 = Monday ... 6 = Sunday; empty = every day

    bool operator==(const Promotion&) const = default;
};

struct QuoteContext {
    std::vector<Bundle> bundles;
    std::vector<Coupon> coupons;
    std::map<std::string, std::int64_t> customer_coupon_uses;
    Timestamp now = 0;
};

struct PriceQuote {
    money::Money subtotal;
    money::Money bundle_savings;
    std::optional<std::string> coupon_code;
    money::Money coupon_discount;
    money::Money total;

    bool operator==(const PriceQuote&) const = default;
};

// ---- discount.cpp -----------------------------------------------------------
money::Money line_subtotal(const LineItem& line);
money::Money order_subtotal(const std::vector<LineItem>& lines, std::string_view currency);
// Price after taking `basis_points` off (half-up rounding of the discount).
money::Money apply_percent_discount(const money::Money& price, std::int64_t basis_points);
// Price after a fixed amount off, never below zero.
money::Money apply_fixed_discount(const money::Money& price, const money::Money& off);
money::Money percent_discount_amount(const money::Money& price, std::int64_t basis_points);
money::Money cap_discount(const money::Money& discount, const money::Money& cap);
// Splits an order-level discount across lines in proportion to line subtotals.
std::vector<money::Money> prorate_discount(const std::vector<LineItem>& lines,
                                           const money::Money& discount);
// Per-unit price after a line discount (half-even rounding).
money::Money effective_unit_price(const LineItem& line, const money::Money& line_discount);
// Rounds a price up to a "charm" price ending in 9 (19.99, 819.00, 1009.00).
money::Money charm_price(const money::Money& price);
// Gross margin in basis points: (price - cost) / price.
std::int64_t margin_bp(const money::Money& price, const money::Money& cost);
// Raises the price, if needed, so the margin is at least `min_margin_bp`.
money::Money enforce_min_margin(const money::Money& price, const money::Money& cost,
                                std::int64_t min_margin_bp);

// ---- coupon.cpp -------------------------------------------------------------
std::string normalize_coupon_code(std::string_view code);
// Parses data/coupons.csv format.
std::vector<Coupon> parse_coupon_csv(std::string_view csv);
bool is_coupon_active(const Coupon& coupon, Timestamp now);
// Subtotal of the lines the coupon applies to.
money::Money coupon_eligible_subtotal(const Coupon& coupon, const std::vector<LineItem>& lines);
CouponStatus validate_coupon(const Coupon& coupon, const std::vector<LineItem>& lines,
                             Timestamp now, std::int64_t customer_uses);
// Discount the coupon gives on these lines (does not check validity).
money::Money coupon_discount(const Coupon& coupon, const std::vector<LineItem>& lines);
// Code of the valid coupon with the largest discount (ties: alphabetical). A free-shipping
// coupon is only chosen when no coupon gives a money discount.
std::optional<std::string> best_coupon(const std::vector<Coupon>& coupons,
                                       const std::vector<LineItem>& lines, Timestamp now,
                                       const std::map<std::string, std::int64_t>& customer_uses);
// Records one use; returns uses left (-1 for unlimited).
std::int64_t redeem_coupon(Coupon& coupon);
// Stacking rule: the single best percent coupon plus every fixed coupon, capped at the
// order subtotal.
money::Money combine_coupons(const std::vector<Coupon>& coupons,
                             const std::vector<LineItem>& lines);

// ---- tiered.cpp -------------------------------------------------------------
// Parses "1:100.00,10:90.00,50:80.00" in the given currency.
std::vector<PriceTier> parse_tiers(std::string_view spec, std::string_view currency);
bool validate_tiers(const std::vector<PriceTier>& tiers);
PriceTier tier_for_quantity(const std::vector<PriceTier>& tiers, std::int64_t qty);
// All-units pricing: every unit gets the price of the tier the quantity falls into.
money::Money tiered_unit_price(const std::vector<PriceTier>& tiers, std::int64_t qty);
// Graduated pricing: each band of units is priced at its own tier.
money::Money graduated_total(const std::vector<PriceTier>& tiers, std::int64_t qty);
std::optional<NextTier> next_tier_savings(const std::vector<PriceTier>& tiers, std::int64_t qty);
// Discount of the applicable tier relative to the base tier, in basis points.
std::int64_t volume_discount_bp(const std::vector<PriceTier>& tiers, std::int64_t qty);

// ---- bundle.cpp -------------------------------------------------------------
// Number of complete bundles the lines contain.
std::int64_t bundle_applies(const Bundle& bundle, const std::vector<LineItem>& lines);
money::Money bundle_savings(const Bundle& bundle, const std::vector<LineItem>& lines);
// Greedily applies bundles, best per-bundle saving first.
BundleResult apply_bundles(const std::vector<Bundle>& bundles, const std::vector<LineItem>& lines);
// "Buy x get y free" on a single SKU; returns the discount.
money::Money buy_x_get_y(const std::vector<LineItem>& lines, std::string_view sku, std::int64_t x,
                         std::int64_t y);
// "Buy n, cheapest free": in every group of n units (priciest first) the cheapest is free.
money::Money cheapest_item_free(const std::vector<LineItem>& lines, std::int64_t every_n);

// ---- promotion.cpp ----------------------------------------------------------
bool promotion_active(const Promotion& promotion, Timestamp now, int tz_offset_minutes);
// Bundles first, then the best coupon on the full basket (capped at what is left).
PriceQuote quote_order(const std::vector<LineItem>& lines, const QuoteContext& context);
// Matches the lowest competitor price but never goes below `floor`.
money::Money price_match(const money::Money& ours, const std::vector<money::Money>& competitors,
                         const money::Money& floor);

}  // namespace shopcore::pricing
