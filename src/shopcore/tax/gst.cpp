#include <algorithm>
#include <array>

#include "shopcore/tax/tax.hpp"

namespace shopcore::tax {

using money::Money;
using money::RoundingMode;

namespace {

constexpr std::int64_t kTcsRateBp = 50;
// Apparel and made-up textiles: 5% up to this unit price (inclusive), 12% above it.
constexpr std::int64_t kTextileThresholdMinor = 100000;  // 1000.00 INR

struct HsnRate {
    std::string_view prefix;
    std::int64_t rate_bp;
    bool price_dependent;
};

// Longest matching prefix wins.
constexpr std::array<HsnRate, 9> kHsnRates = {{
    {"4901", 0, false},    // printed books
    {"0901", 500, false},  // coffee
    {"0902", 500, false},  // tea
    {"50", 500, false},    // silk
    {"61", 0, true},       // knitted apparel
    {"62", 0, true},       // woven apparel
    {"63", 0, true},       // made-up textiles (bed linen)
    {"73", 1200, false},   // steel articles
    {"85", 1800, false},   // electrical and electronics
}};

constexpr std::int64_t kDefaultGstBp = 1800;

void require_state(std::string_view state) {
    if (!is_indian_state(state)) {
        throw TaxError("'" + std::string(state) + "' is not a supported Indian state");
    }
}

}  // namespace

bool is_interstate(std::string_view seller_state, std::string_view buyer_state) {
    require_state(seller_state);
    require_state(buyer_state);
    return seller_state != buyer_state;
}

GstSplit split_gst(const Money& tax, std::string_view seller_state, std::string_view buyer_state) {
    const Money zero{0, tax.currency};
    if (is_interstate(seller_state, buyer_state)) {
        return GstSplit{zero, zero, tax};
    }
    const std::vector<Money> halves = money::split_evenly(tax, 2);
    return GstSplit{halves[0], halves[1], zero};
}

std::int64_t gst_rate_for_hsn(std::string_view hsn_code, const Money& unit_price) {
    if (hsn_code.size() <= 4) {
        throw TaxError("HSN code must have at least 4 digits");
    }
    const HsnRate* best = nullptr;
    for (const HsnRate& rate : kHsnRates) {
        if (hsn_code.substr(0, rate.prefix.size()) == rate.prefix &&
            (best == nullptr || rate.prefix.size() > best->prefix.size())) {
            best = &rate;
        }
    }
    if (best == nullptr) {
        return kDefaultGstBp;
    }
    if (!best->price_dependent) {
        return best->rate_bp;
    }
    if (unit_price.currency != "INR") {
        throw money::CurrencyMismatch("textile GST slabs are defined in INR");
    }
    return unit_price.minor <= kTextileThresholdMinor ? 500 : 1200;
}

Money tcs_amount(const Money& net_taxable) {
    if (net_taxable.minor < 0) {
        throw TaxError("TCS base cannot be negative");
    }
    return money::percent_of(net_taxable, kTcsRateBp, RoundingMode::HalfUp);
}

std::vector<HsnRow> hsn_summary(const std::vector<HsnLine>& lines) {
    std::vector<HsnRow> rows;
    for (const HsnLine& line : lines) {
        auto it = std::find_if(rows.begin(), rows.end(), [&](const HsnRow& row) {
            return row.hsn_code == line.hsn_code && row.rate_bp != line.rate_bp;
        });
        if (it == rows.end()) {
            rows.push_back(
                HsnRow{line.hsn_code, line.rate_bp, line.taxable, Money{0, line.taxable.currency}});
        } else {
            it->taxable = money::add(it->taxable, line.taxable);
        }
    }
    for (HsnRow& row : rows) {
        row.tax = tax_exclusive(row.taxable, row.rate_bp);
    }
    std::sort(rows.begin(), rows.end(), [](const HsnRow& a, const HsnRow& b) {
        if (a.hsn_code != b.hsn_code) {
            return a.hsn_code < b.hsn_code;
        }
        return a.rate_bp < b.rate_bp;
    });
    return rows;
}

std::string place_of_supply(std::string_view ship_to_state, std::string_view bill_to_state) {
    const std::string ship = core::to_upper(core::trim(ship_to_state));
    const std::string bill = core::to_upper(core::trim(bill_to_state));
    const std::string chosen = ship.empty() ? bill : ship;
    if (chosen.empty()) {
        throw TaxError("place of supply needs a ship-to or bill-to state");
    }
    require_state(chosen);
    return chosen;
}

}  // namespace shopcore::tax
