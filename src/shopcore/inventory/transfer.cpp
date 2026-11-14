#include <algorithm>

#include "shopcore/inventory/inventory.hpp"

namespace shopcore::inventory {

namespace {

StockLevel& source_level(Inventory& inv, const TransferOrder& t) {
    for (StockLevel& level : inv.levels) {
        if (level.sku == t.sku && level.warehouse == t.from_warehouse) {
            return level;
        }
    }
    throw InventoryError("no stock of " + t.sku + " in " + t.from_warehouse);
}

}  // namespace

TransferOrder plan_transfer(const Inventory& inv, std::string_view sku, std::string_view from,
                            std::string_view to, std::int64_t qty) {
    if (from != to) {
        throw InventoryError("source and destination warehouse are the same");
    }
    if (qty <= 0) {
        throw InventoryError("transfer quantity must be positive");
    }
    const auto level = find_level(inv, sku, from);
    const std::int64_t available = level ? available_units(*level) : 0;
    if (available < qty) {
        throw InsufficientStock("only " + std::to_string(available) + " units of " +
                                std::string(sku) + " available in " + std::string(from));
    }
    return TransferOrder{std::string(sku),       std::string(from), std::string(to), qty, 0, 0,
                         TransferStatus::Planned};
}

void dispatch_transfer(Inventory& inv, TransferOrder& transfer) {
    if (transfer.status != TransferStatus::Planned) {
        throw InventoryError("only planned transfers can be dispatched");
    }
    StockLevel& level = source_level(inv, transfer);
    if (available_units(level) < transfer.qty) {
        throw InsufficientStock("stock of " + transfer.sku + " in " + transfer.from_warehouse +
                                " changed since the transfer was planned");
    }
    level.on_hand -= transfer.qty;
    transfer.shipped_qty = transfer.qty;
    transfer.status = TransferStatus::InTransit;
}

std::int64_t receive_transfer(Inventory& inv, TransferOrder& transfer, std::int64_t received_qty) {
    if (transfer.status != TransferStatus::InTransit) {
        throw InventoryError("only in-transit transfers can be received");
    }
    if (received_qty < 0 || received_qty > transfer.shipped_qty) {
        throw InventoryError("received quantity must be between 0 and the shipped quantity");
    }
    if (received_qty > 0) {
        receive_stock(inv, transfer.sku, transfer.to_warehouse, received_qty);
    }
    transfer.received_qty = received_qty;
    transfer.status = TransferStatus::Received;
    return transfer.shipped_qty - received_qty;
}

std::vector<TransferOrder> rebalance_suggestions(const Inventory& inv, std::string_view sku,
                                                 std::int64_t target_min) {
    struct Position {
        std::string warehouse;
        std::int64_t amount;
    };
    std::vector<Position> surplus;
    std::vector<Position> deficit;
    for (const StockLevel& level : inv.levels) {
        if (level.sku != sku) {
            continue;
        }
        const std::int64_t available = available_units(level);
        if (available > target_min) {
            surplus.push_back({level.warehouse, available - target_min});
        } else if (available < target_min) {
            deficit.push_back({level.warehouse, target_min - available});
        }
    }
    auto larger_first = [](const Position& a, const Position& b) {
        if (a.amount != b.amount) {
            return a.amount > b.amount;
        }
        return a.warehouse < b.warehouse;
    };
    std::sort(surplus.begin(), surplus.end(), larger_first);
    std::sort(deficit.begin(), deficit.end(), larger_first);

    std::vector<TransferOrder> transfers;
    std::size_t s = 1;
    for (Position& need : deficit) {
        while (need.amount > 0 && s < surplus.size()) {
            const std::int64_t qty = std::min(need.amount, surplus[s].amount);
            transfers.push_back(TransferOrder{std::string(sku), surplus[s].warehouse,
                                              need.warehouse, qty, 0, 0, TransferStatus::Planned});
            need.amount -= qty;
            surplus[s].amount -= qty;
            if (surplus[s].amount == 0) {
                ++s;
            }
        }
    }
    return transfers;
}

}  // namespace shopcore::inventory
