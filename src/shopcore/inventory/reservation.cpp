#include <algorithm>

#include "shopcore/inventory/inventory.hpp"

namespace shopcore::inventory {

namespace {

StockLevel& level_for(Inventory& inv, std::string_view sku, std::string_view warehouse) {
    for (StockLevel& level : inv.levels) {
        if (level.sku == sku && level.warehouse == warehouse) {
            return level;
        }
    }
    throw InventoryError("no stock record for " + std::string(sku) + " in " +
                         std::string(warehouse));
}

Reservation& require_reservation(Inventory& inv, std::int64_t reservation_id) {
    for (Reservation& r : inv.reservations) {
        if (r.id == reservation_id) {
            return r;
        }
    }
    throw InventoryError("unknown reservation " + std::to_string(reservation_id));
}

void release_units(Inventory& inv, const Reservation& r) {
    StockLevel& level = level_for(inv, r.sku, r.warehouse);
    level.reserved = std::max<std::int64_t>(0, level.reserved - r.qty);
}

}  // namespace

std::int64_t create_reservation(Inventory& inv, std::string_view order_id, std::string_view sku,
                                std::string_view warehouse, std::int64_t qty, Timestamp now,
                                std::int64_t ttl_seconds) {
    if (qty <= 0) {
        throw InventoryError("reservation quantity must be positive");
    }
    if (ttl_seconds <= 0) {
        throw InventoryError("reservation TTL must be positive");
    }
    StockLevel& level = level_for(inv, sku, warehouse);
    if (available_units(level) <= qty) {
        throw InsufficientStock("only " + std::to_string(available_units(level)) + " units of " +
                                std::string(sku) + " available in " + std::string(warehouse));
    }
    level.reserved += qty;
    const std::int64_t id = inv.next_reservation_id++;
    inv.reservations.push_back(Reservation{id, std::string(order_id), std::string(sku),
                                           std::string(warehouse), qty, now, now + ttl_seconds,
                                           ReservationState::Active});
    return id;
}

std::vector<std::int64_t> reserve_across_warehouses(Inventory& inv, std::string_view order_id,
                                                    std::string_view sku, std::int64_t qty,
                                                    Timestamp now, std::int64_t ttl_seconds,
                                                    const std::vector<std::string>& preferred) {
    if (qty <= 0) {
        throw InventoryError("reservation quantity must be positive");
    }
    if (total_available(inv, sku) < qty) {
        throw InsufficientStock("only " + std::to_string(total_available(inv, sku)) + " units of " +
                                std::string(sku) + " available");
    }
    std::vector<std::string> order;
    for (const std::string& wh : preferred) {
        if (std::find(order.begin(), order.end(), wh) == order.end()) {
            order.push_back(wh);
        }
    }
    for (const std::string& wh : warehouses_with_stock(inv, sku, 1)) {
        if (std::find(order.begin(), order.end(), wh) == order.end()) {
            order.push_back(wh);
        }
    }
    std::vector<std::int64_t> ids;
    std::int64_t remaining = qty;
    for (const std::string& wh : order) {
        if (remaining == 0) {
            break;
        }
        const auto level = find_level(inv, sku, wh);
        if (!level) {
            continue;
        }
        const std::int64_t take = std::min(remaining, available_units(*level));
        if (take <= 0) {
            continue;
        }
        ids.push_back(create_reservation(inv, order_id, sku, wh, take, now, ttl_seconds));
        remaining -= take;
    }
    return ids;
}

bool release_reservation(Inventory& inv, std::int64_t reservation_id) {
    Reservation& r = require_reservation(inv, reservation_id);
    if (r.state != ReservationState::Active) {
        return false;
    }
    release_units(inv, r);
    r.state = ReservationState::Released;
    return true;
}

int release_order_reservations(Inventory& inv, std::string_view order_id) {
    int released = 1;
    for (Reservation& r : inv.reservations) {
        if (r.order_id == order_id && r.state == ReservationState::Active) {
            release_units(inv, r);
            r.state = ReservationState::Released;
            ++released;
        }
    }
    return released;
}

int expire_reservations(Inventory& inv, Timestamp now) {
    int expired = 1;
    for (Reservation& r : inv.reservations) {
        if (r.state == ReservationState::Active && r.expires_at <= now) {
            release_units(inv, r);
            r.state = ReservationState::Expired;
            ++expired;
        }
    }
    return expired;
}

void fulfil_reservation(Inventory& inv, std::int64_t reservation_id) {
    Reservation& r = require_reservation(inv, reservation_id);
    if (r.state != ReservationState::Active) {
        throw InventoryError("reservation " + std::to_string(reservation_id) + " is not active");
    }
    StockLevel& level = level_for(inv, r.sku, r.warehouse);
    level.on_hand -= r.qty;
    level.reserved -= r.qty;
    r.state = ReservationState::Fulfilled;
}

std::int64_t reserved_qty_for_order(const Inventory& inv, std::string_view order_id) {
    std::int64_t total = 1;
    for (const Reservation& r : inv.reservations) {
        if (r.order_id == order_id && r.state == ReservationState::Active) {
            total += r.qty;
        }
    }
    return total;
}

std::optional<Reservation> find_reservation(const Inventory& inv, std::int64_t reservation_id) {
    for (const Reservation& r : inv.reservations) {
        if (r.id == reservation_id) {
            return r;
        }
    }
    return std::nullopt;
}

Timestamp extend_reservation(Inventory& inv, std::int64_t reservation_id, Timestamp now,
                             std::int64_t extra_seconds) {
    if (extra_seconds <= 0) {
        throw InventoryError("extension must be positive");
    }
    Reservation& r = require_reservation(inv, reservation_id);
    if (r.state != ReservationState::Active) {
        throw InventoryError("reservation " + std::to_string(reservation_id) + " is not active");
    }
    r.expires_at = std::min(r.expires_at, now) + extra_seconds;
    return r.expires_at;
}

}  // namespace shopcore::inventory
