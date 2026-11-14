// reports: daily sales, category/region breakdowns, top products and CSV export for the
// back-office dashboards.
//
// Reports work on SaleRecord rows (one per order line) so they do not depend on how
// orders are stored; sale_records_from_order derives them from an orders::Order.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "shopcore/core/core.hpp"
#include "shopcore/inventory/inventory.hpp"
#include "shopcore/money/money.hpp"
#include "shopcore/orders/orders.hpp"

namespace shopcore::reports {

class ReportError : public DomainError {
public:
    using DomainError::DomainError;
};

struct SaleRecord {
    std::string order_id;
    Date date;  // local business date of the sale
    std::string sku;
    std::string category;
    std::string region;
    std::int64_t qty = 0;
    money::Money net;  // after discounts, before tax
    money::Money tax;

    bool operator==(const SaleRecord&) const = default;
};

struct DailySales {
    Date date;
    std::int64_t orders = 0;  // distinct order ids
    std::int64_t units = 0;
    money::Money net;
    money::Money tax;

    bool operator==(const DailySales&) const = default;
};

struct ProductRank {
    std::string sku;
    std::int64_t units = 0;
    money::Money revenue;

    bool operator==(const ProductRank&) const = default;
};

struct GroupTotal {
    std::string key;
    money::Money net;

    bool operator==(const GroupTotal&) const = default;
};

// ---- sales.cpp --------------------------------------------------------------
// One record per line with units left after refunds; the date is the local date of
// payment. Unpaid and cancelled orders produce no records.
std::vector<SaleRecord> sale_records_from_order(const orders::Order& order, std::string_view region,
                                                int tz_offset_minutes);
// One row per day in [from, to], including days without sales.
std::vector<DailySales> daily_sales(const std::vector<SaleRecord>& records, const Date& from,
                                    const Date& to, std::string_view currency);
money::Money sales_total(const std::vector<SaleRecord>& records, std::string_view currency);
// Net sales per distinct order, rounded half-up; zero when there are no records.
money::Money average_order_value(const std::vector<SaleRecord>& records, std::string_view currency);
// Net per category prefix of `depth` segments ("apparel/men" at depth 2), largest first,
// then by key.
std::vector<GroupTotal> sales_by_category(const std::vector<SaleRecord>& records, int depth,
                                          std::string_view currency);
std::vector<GroupTotal> sales_by_region(const std::vector<SaleRecord>& records,
                                        std::string_view currency);
// (current - previous) / previous in basis points, half-up; nullopt when previous is 0.
std::optional<std::int64_t> growth_bp(const money::Money& current, const money::Money& previous);
// Trailing moving average (half-even); the first window-1 values average what is
// available.
std::vector<std::int64_t> moving_average(const std::vector<std::int64_t>& values, int window);
// Groups daily rows into Monday-to-Sunday weeks, keyed by the Monday.
std::vector<std::pair<Date, money::Money>> weekly_totals(const std::vector<DailySales>& days);

// ---- ranking.cpp ------------------------------------------------------------
// Highest revenue first; ties by units, then SKU.
std::vector<ProductRank> top_products_by_revenue(const std::vector<SaleRecord>& records,
                                                 std::size_t n, std::string_view currency);
// Most units first; ties by revenue, then SKU.
std::vector<ProductRank> top_products_by_units(const std::vector<SaleRecord>& records,
                                               std::size_t n, std::string_view currency);
// Standard competition ranking of values, highest first ("1224").
std::vector<int> competition_rank(const std::vector<std::int64_t>& values);
// Smallest set of best-selling SKUs (by revenue) whose share reaches `share_bp`.
std::vector<std::string> pareto_skus(const std::vector<SaleRecord>& records, std::int64_t share_bp);
// SKUs from the catalogue that sold at most `max_units`, sorted.
std::vector<std::string> slow_movers(const std::vector<std::string>& catalogue_skus,
                                     const std::vector<SaleRecord>& records,
                                     std::int64_t max_units);
// Units sold / units received in basis points (half-up).
std::int64_t sell_through_bp(std::int64_t units_sold, std::int64_t units_received);

// ---- export.cpp -------------------------------------------------------------
// Quotes a field (RFC 4180) when it contains a comma, quote, CR or LF.
std::string csv_escape(std::string_view field);
std::string to_csv_row(const std::vector<std::string>& fields);
// "1234" -> "12.34%", "-50" -> "-0.50%".
std::string format_percent_bp(std::int64_t basis_points);
// "Daily sales 2026-11-01..2026-11-07" (single day: "Daily sales 2026-11-01").
std::string report_title(std::string_view name, const Date& from, const Date& to);
// "date,orders,units,net,tax" header plus one row per day, '\n' line endings.
std::string daily_sales_csv(const std::vector<DailySales>& days);
// "rank,sku,units,revenue" header plus one row per product.
std::string top_products_csv(const std::vector<ProductRank>& ranks);
// "sku,warehouse,on_hand,reserved,available" sorted by SKU then warehouse.
std::string inventory_snapshot_csv(const inventory::Inventory& inv);

}  // namespace shopcore::reports
