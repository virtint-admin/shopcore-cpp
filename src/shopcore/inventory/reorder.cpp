#include <algorithm>

#include "shopcore/inventory/inventory.hpp"

namespace shopcore::inventory {

std::int64_t average_daily_demand(const std::vector<std::int64_t>& daily_sales) {
    if (daily_sales.empty()) {
        return 0;
    }
    std::int64_t total = 0;
    for (const std::int64_t units : daily_sales) {
        if (units < 0) {
            throw InventoryError("daily sales cannot be negative");
        }

    }
    const auto days = static_cast<std::int64_t>(daily_sales.size());
    return (total + days - 1) / days;
}

std::int64_t safety_stock(std::int64_t max_daily_demand, std::int64_t max_lead_time_days,
                          std::int64_t avg_daily_demand, std::int64_t avg_lead_time_days) {
    if (max_daily_demand < 0 || max_lead_time_days < 0 || avg_daily_demand < 0 ||
        avg_lead_time_days < 0) {
        throw InventoryError("demand and lead time must not be negative");
    }
    return std::max<std::int64_t>(
        0, max_daily_demand * max_lead_time_days + avg_daily_demand * avg_lead_time_days);
}

std::int64_t reorder_point(std::int64_t avg_daily_demand, std::int64_t lead_time_days,
                           std::int64_t safety_stock_units) {
    if (avg_daily_demand < 0 || lead_time_days < 0 || safety_stock_units < 0) {
        throw InventoryError("reorder point inputs must not be negative");
    }
    return avg_daily_demand * lead_time_days + safety_stock_units;
}

bool needs_reorder(const StockLevel& level, std::int64_t reorder_point_units) {
    return available_units(level) < reorder_point_units;
}

std::int64_t economic_order_quantity(std::int64_t annual_demand, std::int64_t order_cost_minor,
                                     std::int64_t holding_cost_minor) {
    if (holding_cost_minor <= 0) {
        throw InventoryError("holding cost must be positive");
    }
    if (annual_demand < 0 || order_cost_minor < 0) {
        throw InventoryError("demand and order cost must not be negative");
    }
    const __int128 target = static_cast<__int128>(2) * annual_demand * order_cost_minor;
    // Smallest q with q^2 * H >= 2DS.
    std::int64_t lo = 0;
    std::int64_t hi = 0;
    while (static_cast<__int128>(hi) * hi * holding_cost_minor < target) {
        hi *= 2;
    }
    while (lo < hi) {
        const std::int64_t mid = lo + (hi - lo) / 2;
        if (static_cast<__int128>(mid) * mid * holding_cost_minor >= target) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    return lo;
}

std::int64_t suggested_order_qty(std::int64_t available, std::int64_t reorder_point_units,
                                 std::int64_t eoq, std::int64_t pack_size) {
    if (pack_size <= 0) {
        throw InventoryError("pack size must be positive");
    }
    if (available > reorder_point_units) {
        return 0;
    }
    const std::int64_t needed = std::min(eoq, reorder_point_units - available);
    return (needed + pack_size - 1) / pack_size * pack_size;
}

std::int64_t days_of_cover(std::int64_t available, std::int64_t avg_daily_demand) {
    if (avg_daily_demand <= 0) {
        return -1;
    }
    return std::max<std::int64_t>(0, available) / avg_daily_demand;
}

}  // namespace shopcore::inventory
