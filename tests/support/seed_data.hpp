// Raw seed data from data/*.csv, embedded at configure time (see CMakeLists.txt).
#pragma once

#include <string_view>

namespace shopcore::testing::seed {

std::string_view products_csv();
std::string_view tax_rules_csv();
std::string_view rate_table_csv();
std::string_view stock_csv();
std::string_view coupons_csv();
std::string_view fx_rates_csv();
std::string_view settlements_csv();

}  // namespace shopcore::testing::seed
