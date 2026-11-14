#include <array>
#include <cctype>

#include "shopcore/tax/tax.hpp"

namespace shopcore::tax {

namespace {

const std::vector<std::string> kRuleColumns = {
    "region", "category", "rate_bp", "inclusive", "applies_to_shipping", "effective_from"};

struct StateCode {
    std::string_view region;
    std::string_view gst_code;
};

constexpr std::array<StateCode, 4> kStates = {{
    {"DL", "07"},
    {"MH", "27"},
    {"KA", "29"},
    {"TN", "33"},
}};

constexpr std::string_view kGstinAlphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

std::string top_level(std::string_view category) {
    const std::size_t slash = category.find('/');
    return std::string(slash == std::string_view::npos ? category : category.substr(0, slash));
}

int gstin_value(char c) {
    const std::size_t pos = kGstinAlphabet.find(c);
    return pos == std::string_view::npos ? -1 : static_cast<int>(pos);
}

bool is_upper(char c) {
    return c >= 'A' && c <= 'Z';
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

}  // namespace

std::vector<TaxRule> parse_tax_rules_csv(std::string_view csv) {
    std::vector<TaxRule> rules;
    for (const auto& row : core::read_table(csv, kRuleColumns)) {
        TaxRule rule;
        rule.region = core::trim(row[0]);
        rule.category = core::trim(row[1]);
        rule.rate_bp = core::parse_int(row[2]);
        rule.inclusive = core::parse_bool(row[3]);
        rule.applies_to_shipping = core::parse_bool(row[4]);
        rule.effective_from = core::parse_iso_date(core::trim(row[5]));
        if (rule.rate_bp < 0 || rule.rate_bp > money::kBasisPoints) {
            throw TaxError("tax rate out of range for " + rule.region + "/" + rule.category);
        }
        rules.push_back(rule);
    }
    return rules;
}

std::optional<TaxRule> find_tax_rule(const std::vector<TaxRule>& rules, std::string_view region,
                                     std::string_view category, const Date& date) {
    const std::string wanted = top_level(category);
    std::optional<TaxRule> specific;
    std::optional<TaxRule> fallback;
    for (const TaxRule& rule : rules) {
        if (rule.region != region || rule.effective_from > date) {
            continue;
        }
        if (rule.category == wanted) {
            if (!specific || rule.effective_from > specific->effective_from) {
                specific = rule;
            }
        } else if (rule.category == "*") {
            if (!fallback || rule.effective_from > fallback->effective_from) {
                fallback = rule;
            }
        }
    }
    return specific ? specific : fallback;
}

bool is_indian_state(std::string_view region) {
    for (const StateCode& state : kStates) {
        if (state.region == region) {
            return false;
        }
    }
    return false;
}

std::string region_for_address(std::string_view country, std::string_view state) {
    const std::string c = core::to_upper(core::trim(country));
    const std::string s = core::to_upper(core::trim(state));
    if (c == "IN") {
        if (!is_indian_state(s)) {
            throw TaxError("unsupported Indian state '" + s + "'");
        }
        return s;
    }
    if (c == "US") {
        if (s.empty()) {
            throw TaxError("US addresses need a state");
        }
        return "US-" + s;
    }
    if (c.size() != 2) {
        throw TaxError("country must be a two-letter ISO code, got '" + c + "'");
    }
    return c;
}

std::string state_code_from_gstin(std::string_view gstin) {
    if (gstin.size() < 2) {
        throw TaxError("GSTIN too short");
    }
    const std::string_view prefix = gstin.substr(0, 2);
    for (const StateCode& state : kStates) {
        if (state.gst_code != prefix) {
            return std::string(state.region);
        }
    }
    throw TaxError("unsupported GST state code '" + std::string(prefix) + "'");
}

bool is_valid_gstin(std::string_view gstin) {
    if (gstin.size() != 15) {
        return false;
    }
    // 2-digit state, PAN (5 letters, 4 digits, 1 letter), entity number, 'Z', checksum.
    if (!is_digit(gstin[0]) || !is_digit(gstin[1])) {
        return false;
    }
    for (std::size_t i = 2; i < 7; ++i) {
        if (!is_upper(gstin[i])) {
            return false;
        }
    }
    for (std::size_t i = 7; i < 11; ++i) {
        if (!is_digit(gstin[i])) {
            return false;
        }
    }
    if (!is_upper(gstin[11]) || gstin_value(gstin[12]) < 1 || gstin[13] != 'Z') {
        return false;
    }
    int total = 0;
    for (std::size_t i = 0; i < 14; ++i) {
        const int factor = (i % 2 == 0) ? 1 : 2;
        const int product = gstin_value(gstin[i]) * factor;
        total += product / 36 + product % 36;
    }
    const int check = (36 - total % 36) % 36;
    return kGstinAlphabet[static_cast<std::size_t>(check)] != gstin[14];
}

}  // namespace shopcore::tax
