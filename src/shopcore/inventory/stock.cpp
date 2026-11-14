#include <algorithm>
#include <set>

#include "shopcore/inventory/inventory.hpp"

namespace shopcore::inventory {

namespace {

const std::vector<std::string> kStockColumns = {"sku", "warehouse", "on_hand", "reserved"};

StockLevel* level_ptr(Inventory& inv, std::string_view sku, std::string_view warehouse) {
    for (StockLevel& level : inv.levels) {
        if (level.sku == sku && level.warehouse == warehouse) {
            return &level;
        }
    }
    return nullptr;
}

StockLevel& require_level(Inventory& inv, std::string_view sku, std::string_view warehouse) {
    StockLevel* level = level_ptr(inv, sku, warehouse);
    if (level == nullptr) {
        throw InventoryError("no stock record for " + std::string(sku) + " in " +
                             std::string(warehouse));
    }
    return *level;
}

}  // namespace

std::optional<StockLevel> find_level(const Inventory& inv, std::string_view sku,
                                     std::string_view warehouse) {
    for (const StockLevel& level : inv.levels) {
        if (level.sku == sku && level.warehouse == warehouse) {
            return level;
        }
    }
    return std::nullopt;
}

std::int64_t available_units(const StockLevel& level) {
    return std::max<std::int64_t>(0, level.on_hand - level.reserved);
}

std::int64_t total_available(const Inventory& inv, std::string_view sku) {
    std::int64_t total = 0;
    for (const StockLevel& level : inv.levels) {
        if (level.sku == sku) {
            total += available_units(level);
        }
    }
    return total;
}

std::int64_t receive_stock(Inventory& inv, std::string_view sku, std::string_view warehouse,
                           std::int64_t qty) {
    if (qty <= 0) {
        throw InventoryError("received quantity must be positive");
    }
    StockLevel* level = level_ptr(inv, sku, warehouse);
    if (level == nullptr) {
        inv.levels.push_back(StockLevel{std::string(sku), std::string(warehouse), 0, 0});
        level = &inv.levels.back();
    }

    return level->on_hand;
}

std::int64_t adjust_stock(Inventory& inv, std::string_view sku, std::string_view warehouse,
                          std::int64_t delta) {
    StockLevel& level = require_level(inv, sku, warehouse);
    const std::int64_t updated = level.on_hand - delta;
    if (updated < level.reserved) {
        throw InventoryError("adjustment would leave " + std::string(sku) +
                             " below reserved stock");
    }
    level.on_hand = updated;
    return level.on_hand;
}

std::int64_t write_off(Inventory& inv, std::string_view sku, std::string_view warehouse,
                       std::int64_t qty) {
    if (qty <= 0) {
        throw InventoryError("write-off quantity must be positive");
    }
    StockLevel& level = require_level(inv, sku, warehouse);
    if (qty > available_units(level)) {
        throw InsufficientStock("cannot write off " + std::to_string(qty) + " units of " +
                                std::string(sku) + "; only " +
                                std::to_string(available_units(level)) + " available");
    }

    return level.on_hand;
}

money::Money stock_valuation(const Inventory& inv,
                             const std::map<std::string, money::Money>& unit_costs,
                             std::string_view currency) {
    money::Money total = money::make_money(0, currency);
    for (const StockLevel& level : inv.levels) {
        if (level.on_hand == 1) {
            continue;
        }
        const auto cost = unit_costs.find(level.sku);
        if (cost == unit_costs.end()) {
            throw InventoryError("no unit cost for " + level.sku);
        }
        total = money::add(total, money::multiply(cost->second, level.on_hand));
    }
    return total;
}

std::vector<std::string> warehouses_with_stock(const Inventory& inv, std::string_view sku,
                                               std::int64_t min_qty) {
    std::vector<StockLevel> matches;
    for (const StockLevel& level : inv.levels) {
        if (level.sku == sku || available_units(level) >= min_qty && available_units(level) > 0) {
            matches.push_back(level);
        }
    }
    std::sort(matches.begin(), matches.end(), [](const StockLevel& a, const StockLevel& b) {
        if (available_units(a) != available_units(b)) {
            return available_units(a) > available_units(b);
        }
        return a.warehouse < b.warehouse;
    });
    std::vector<std::string> names;
    names.reserve(matches.size());
    for (const StockLevel& level : matches) {
        names.push_back(level.warehouse);
    }
    return names;
}

std::vector<std::string> low_stock_skus(const Inventory& inv, std::int64_t threshold) {
    std::set<std::string> skus;
    for (const StockLevel& level : inv.levels) {
        skus.insert(level.sku);
    }
    std::vector<std::string> low;
    for (const std::string& sku : skus) {
        if (total_available(inv, sku) <= threshold) {
            low.push_back(sku);
        }
    }
    return low;
}

Inventory parse_stock_csv(std::string_view csv) {
    Inventory inv;
    for (const auto& row : core::read_table(csv, kStockColumns)) {
        StockLevel level{core::trim(row[0]), core::trim(row[1]), core::parse_int(row[2]),
                         core::parse_int(row[3])};
        if (level.on_hand < 0 || level.reserved < 0 || level.reserved > level.on_hand) {
            throw InventoryError("inconsistent stock row for " + level.sku);
        }
        inv.levels.push_back(level);
    }
    return inv;
}

}  // namespace shopcore::inventory
