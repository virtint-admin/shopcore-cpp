#include "support/fixtures.hpp"

#include <string>

#include "support/seed_data.hpp"

namespace shopcore::testing {

money::Money inr(std::string_view amount) {
    return money::parse_money(std::string(amount) + " INR");
}

money::Money usd(std::string_view amount) {
    return money::parse_money(std::string(amount) + " USD");
}

money::Money money_of(std::string_view text) {
    return money::parse_money(text);
}

Date date_of(std::string_view iso) {
    return core::parse_iso_date(iso);
}

Timestamp ts(std::string_view iso) {
    return core::parse_iso_timestamp(iso);
}

std::vector<catalog::Product> seed_products() {
    return catalog::parse_product_csv(seed::products_csv());
}

std::vector<tax::TaxRule> seed_tax_rules() {
    return tax::parse_tax_rules_csv(seed::tax_rules_csv());
}

inventory::Inventory seed_inventory() {
    return inventory::parse_stock_csv(seed::stock_csv());
}

std::vector<pricing::Coupon> seed_coupons() {
    return pricing::parse_coupon_csv(seed::coupons_csv());
}

std::vector<money::FxRate> seed_fx_rates() {
    std::vector<money::FxRate> rates;
    for (const auto& row : core::read_table(seed::fx_rates_csv(), {"rate"})) {
        rates.push_back(money::parse_fx_rate(row[0]));
    }
    return rates;
}

shipping::RateTable seed_rate_table() {
    return shipping::parse_rate_table_csv(seed::rate_table_csv());
}

std::vector<ledger::Settlement> seed_settlements() {
    return ledger::parse_settlements_csv(seed::settlements_csv());
}

}  // namespace shopcore::testing
