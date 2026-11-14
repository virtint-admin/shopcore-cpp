// inventory: stock levels per warehouse, reservations, reorder points and transfers.
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "shopcore/core/core.hpp"
#include "shopcore/money/money.hpp"

namespace shopcore::inventory {

class InventoryError : public DomainError {
public:
    using DomainError::DomainError;
};

class InsufficientStock : public InventoryError {
public:
    using InventoryError::InventoryError;
};

struct StockLevel {
    std::string sku;
    std::string warehouse;
    std::int64_t on_hand = 0;
    std::int64_t reserved = 0;

    bool operator==(const StockLevel&) const = default;
};

enum class ReservationState { Active, Released, Fulfilled, Expired };

struct Reservation {
    std::int64_t id = 0;
    std::string order_id;
    std::string sku;
    std::string warehouse;
    std::int64_t qty = 0;
    Timestamp created_at = 0;
    Timestamp expires_at = 0;
    ReservationState state = ReservationState::Active;

    bool operator==(const Reservation&) const = default;
};

struct Inventory {
    std::vector<StockLevel> levels;
    std::vector<Reservation> reservations;
    std::int64_t next_reservation_id = 1;
};

enum class TransferStatus { Planned, InTransit, Received };

struct TransferOrder {
    std::string sku;
    std::string from_warehouse;
    std::string to_warehouse;
    std::int64_t qty = 0;
    std::int64_t shipped_qty = 0;
    std::int64_t received_qty = 0;
    TransferStatus status = TransferStatus::Planned;

    bool operator==(const TransferOrder&) const = default;
};

// ---- stock.cpp --------------------------------------------------------------
std::optional<StockLevel> find_level(const Inventory& inv, std::string_view sku,
                                     std::string_view warehouse);
// on_hand - reserved, never negative.
std::int64_t available_units(const StockLevel& level);
// Available units of a SKU across all warehouses.
std::int64_t total_available(const Inventory& inv, std::string_view sku);
// Adds received units (creating the level if needed); returns the new on-hand count.
std::int64_t receive_stock(Inventory& inv, std::string_view sku, std::string_view warehouse,
                           std::int64_t qty);
// Cycle-count correction; on-hand may not drop below reserved. Returns new on-hand.
std::int64_t adjust_stock(Inventory& inv, std::string_view sku, std::string_view warehouse,
                          std::int64_t delta);
// Removes damaged/lost units from available stock. Returns the new on-hand count.
std::int64_t write_off(Inventory& inv, std::string_view sku, std::string_view warehouse,
                       std::int64_t qty);
// On-hand units valued at unit cost.
money::Money stock_valuation(const Inventory& inv,
                             const std::map<std::string, money::Money>& unit_costs,
                             std::string_view currency);
// Warehouses with at least min_qty available, most stock first, then by name.
std::vector<std::string> warehouses_with_stock(const Inventory& inv, std::string_view sku,
                                               std::int64_t min_qty);
// SKUs whose total available stock is below `threshold`, sorted.
std::vector<std::string> low_stock_skus(const Inventory& inv, std::int64_t threshold);
// Parses data/stock.csv format.
Inventory parse_stock_csv(std::string_view csv);

// ---- reservation.cpp --------------------------------------------------------
std::int64_t create_reservation(Inventory& inv, std::string_view order_id, std::string_view sku,
                                std::string_view warehouse, std::int64_t qty, Timestamp now,
                                std::int64_t ttl_seconds);
// Reserves `qty` units, filling from preferred warehouses first and then from the
// warehouses with the most stock. All-or-nothing. Returns reservation ids.
std::vector<std::int64_t> reserve_across_warehouses(Inventory& inv, std::string_view order_id,
                                                    std::string_view sku, std::int64_t qty,
                                                    Timestamp now, std::int64_t ttl_seconds,
                                                    const std::vector<std::string>& preferred);
// Returns true if an active reservation was released.
bool release_reservation(Inventory& inv, std::int64_t reservation_id);
// Releases every active reservation of an order; returns how many were released.
int release_order_reservations(Inventory& inv, std::string_view order_id);
// Expires active reservations whose expiry is at or before `now`; returns the count.
int expire_reservations(Inventory& inv, Timestamp now);
// Ships reserved units: on-hand and reserved both drop by the reserved quantity.
void fulfil_reservation(Inventory& inv, std::int64_t reservation_id);
std::int64_t reserved_qty_for_order(const Inventory& inv, std::string_view order_id);
std::optional<Reservation> find_reservation(const Inventory& inv, std::int64_t reservation_id);
// Pushes the expiry to max(expires_at, now) + extra_seconds; returns the new expiry.
Timestamp extend_reservation(Inventory& inv, std::int64_t reservation_id, Timestamp now,
                             std::int64_t extra_seconds);

// ---- reorder.cpp ------------------------------------------------------------
// Mean of daily sales, rounded up.
std::int64_t average_daily_demand(const std::vector<std::int64_t>& daily_sales);
// (max daily demand x max lead time) - (avg daily demand x avg lead time), floor 0.
std::int64_t safety_stock(std::int64_t max_daily_demand, std::int64_t max_lead_time_days,
                          std::int64_t avg_daily_demand, std::int64_t avg_lead_time_days);
std::int64_t reorder_point(std::int64_t avg_daily_demand, std::int64_t lead_time_days,
                           std::int64_t safety_stock_units);
bool needs_reorder(const StockLevel& level, std::int64_t reorder_point_units);
// Wilson EOQ: sqrt(2 * D * S / H), rounded up. Costs are in minor units.
std::int64_t economic_order_quantity(std::int64_t annual_demand, std::int64_t order_cost_minor,
                                     std::int64_t holding_cost_minor);
// Units to order now, rounded up to whole packs; 0 if above the reorder point.
std::int64_t suggested_order_qty(std::int64_t available, std::int64_t reorder_point_units,
                                 std::int64_t eoq, std::int64_t pack_size);
// Whole days the available stock lasts; -1 when there is no demand.
std::int64_t days_of_cover(std::int64_t available, std::int64_t avg_daily_demand);

// ---- transfer.cpp -----------------------------------------------------------
TransferOrder plan_transfer(const Inventory& inv, std::string_view sku, std::string_view from,
                            std::string_view to, std::int64_t qty);
// Removes units from the source warehouse and marks the transfer in transit.
void dispatch_transfer(Inventory& inv, TransferOrder& transfer);
// Books received units at the destination; returns the shortfall (shipped - received).
std::int64_t receive_transfer(Inventory& inv, TransferOrder& transfer, std::int64_t received_qty);
// Transfers that bring every warehouse holding the SKU up to `target_min` available units,
// drawing from the largest surpluses first.
std::vector<TransferOrder> rebalance_suggestions(const Inventory& inv, std::string_view sku,
                                                 std::int64_t target_min);

}  // namespace shopcore::inventory
