// Shared helpers for the ShopCore GoogleTest suites: terse money/date constructors and
// the parsed seed data set (data/*.csv, embedded at configure time).
#pragma once

#include <string_view>
#include <vector>

#include "shopcore/catalog/catalog.hpp"
#include "shopcore/core/core.hpp"
#include "shopcore/inventory/inventory.hpp"
#include "shopcore/ledger/ledger.hpp"
#include "shopcore/money/money.hpp"
#include "shopcore/pricing/pricing.hpp"
#include "shopcore/shipping/shipping.hpp"
#include "shopcore/tax/tax.hpp"

namespace shopcore::testing {

// inr("799.00") == Money{79900, "INR"}
money::Money inr(std::string_view amount);
money::Money usd(std::string_view amount);
// money_of("12.50 EUR")
money::Money money_of(std::string_view text);
// date_of("2026-11-09")
Date date_of(std::string_view iso);
// ts("2026-11-09T10:00:00+05:30")
Timestamp ts(std::string_view iso);

inline constexpr int kIst = 330;  // India Standard Time, minutes east of UTC

std::vector<catalog::Product> seed_products();
std::vector<tax::TaxRule> seed_tax_rules();
inventory::Inventory seed_inventory();
std::vector<pricing::Coupon> seed_coupons();
std::vector<money::FxRate> seed_fx_rates();
shipping::RateTable seed_rate_table();
std::vector<ledger::Settlement> seed_settlements();

}  // namespace shopcore::testing
