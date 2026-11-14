// tax: regional tax rules (Indian GST by state, UK VAT, Singapore GST, California sales
// tax), tax-inclusive vs tax-exclusive pricing, and GST invoice helpers.
//
// Region codes: Indian states use their two-letter code (KA, MH, DL, TN); other regions
// use the ISO country code (GB, SG) or country-state (US-CA).
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "shopcore/core/core.hpp"
#include "shopcore/money/money.hpp"

namespace shopcore::tax {

class TaxError : public DomainError {
public:
    using DomainError::DomainError;
};

class TaxRuleNotFound : public TaxError {
public:
    using TaxError::TaxError;
};

struct TaxRule {
    std::string region;
    std::string category;  // top-level product category, or "*" for the regional default
    std::int64_t rate_bp = 0;
    bool inclusive = false;  // prices in this region already include the tax
    bool applies_to_shipping = false;
    Date effective_from;

    bool operator==(const TaxRule&) const = default;
};

struct TaxableLine {
    std::string sku;
    std::string category;  // product category path
    money::Money amount;   // line amount after discounts, as charged to the customer

    bool operator==(const TaxableLine&) const = default;
};

struct LineTax {
    std::string sku;
    money::Money net;
    money::Money tax;
    std::int64_t rate_bp = 0;

    bool operator==(const LineTax&) const = default;
};

struct TaxBreakdown {
    std::vector<LineTax> lines;
    money::Money goods_net;
    money::Money goods_tax;
    money::Money shipping_net;
    money::Money shipping_tax;
    money::Money gross;  // what the customer pays: nets + taxes

    bool operator==(const TaxBreakdown&) const = default;
};

struct GstSplit {
    money::Money cgst;
    money::Money sgst;
    money::Money igst;

    bool operator==(const GstSplit&) const = default;
};

struct HsnLine {
    std::string hsn_code;
    money::Money taxable;
    std::int64_t rate_bp = 0;
};

struct HsnRow {
    std::string hsn_code;
    std::int64_t rate_bp = 0;
    money::Money taxable;
    money::Money tax;

    bool operator==(const HsnRow&) const = default;
};

// ---- rules.cpp --------------------------------------------------------------
// Parses data/tax_rules.csv format.
std::vector<TaxRule> parse_tax_rules_csv(std::string_view csv);
// Most specific rule (category before "*") in force on `date` (latest effective_from).
std::optional<TaxRule> find_tax_rule(const std::vector<TaxRule>& rules, std::string_view region,
                                     std::string_view category, const Date& date);
bool is_indian_state(std::string_view region);
// ("IN","KA") -> "KA", ("US","CA") -> "US-CA", ("GB","") -> "GB".
std::string region_for_address(std::string_view country, std::string_view state);
// GSTIN state prefix -> region code ("29..." -> "KA").
std::string state_code_from_gstin(std::string_view gstin);
// Format and mod-36 checksum validation of a 15-character GSTIN.
bool is_valid_gstin(std::string_view gstin);

// ---- calculator.cpp ---------------------------------------------------------
// Tax on a tax-exclusive amount.
money::Money tax_exclusive(const money::Money& net, std::int64_t rate_bp);
// Tax contained in a tax-inclusive amount: gross * rate / (1 + rate).
money::Money tax_inclusive_component(const money::Money& gross, std::int64_t rate_bp);
money::Money net_from_gross(const money::Money& gross, std::int64_t rate_bp);
money::Money gross_from_net(const money::Money& net, std::int64_t rate_bp);
LineTax line_tax(const TaxableLine& line, const TaxRule& rule);
// Taxes an order for a region. Shipping is taxed only when the region's default rule
// says so.
TaxBreakdown apply_regional_tax(const std::vector<TaxableLine>& lines, const money::Money& shipping,
                                std::string_view region, const std::vector<TaxRule>& rules,
                                const Date& date);
std::int64_t effective_rate_bp(const money::Money& tax, const money::Money& net);
// Total tax on an invoice, rounding per line or once on the invoice total.
money::Money invoice_tax_total(const std::vector<money::Money>& net_amounts, std::int64_t rate_bp,
                               bool per_line_rounding);

// ---- gst.cpp ----------------------------------------------------------------
bool is_interstate(std::string_view seller_state, std::string_view buyer_state);
// Intra-state: CGST + SGST (half each); inter-state: IGST.
GstSplit split_gst(const money::Money& tax, std::string_view seller_state,
                   std::string_view buyer_state);
// GST rate for an HSN code; apparel and home textiles depend on the unit price.
std::int64_t gst_rate_for_hsn(std::string_view hsn_code, const money::Money& unit_price);
// Tax collected at source by the marketplace (0.5% of net taxable value).
money::Money tcs_amount(const money::Money& net_taxable);
// HSN-wise summary rows for a GST invoice, sorted by HSN code then rate.
std::vector<HsnRow> hsn_summary(const std::vector<HsnLine>& lines);
// Place of supply for goods: the ship-to state, falling back to the bill-to state.
std::string place_of_supply(std::string_view ship_to_state, std::string_view bill_to_state);

}  // namespace shopcore::tax
