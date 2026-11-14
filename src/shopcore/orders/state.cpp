#include <algorithm>
#include <array>
#include <utility>

#include "shopcore/orders/orders.hpp"

namespace shopcore::orders {

using money::Money;
using money::RoundingMode;

namespace {

constexpr std::array<std::pair<OrderStatus, OrderStatus>, 10> kTransitions = {{
    {OrderStatus::Pending, OrderStatus::Paid},
    {OrderStatus::Pending, OrderStatus::Cancelled},
    {OrderStatus::Paid, OrderStatus::Packed},
    {OrderStatus::Paid, OrderStatus::Cancelled},
    {OrderStatus::Packed, OrderStatus::Shipped},
    {OrderStatus::Packed, OrderStatus::Cancelled},
    {OrderStatus::Shipped, OrderStatus::Delivered},
    {OrderStatus::Delivered, OrderStatus::ReturnRequested},
    {OrderStatus::ReturnRequested, OrderStatus::Returned},
    {OrderStatus::ReturnRequested, OrderStatus::Delivered},
}};

constexpr std::array<OrderStatus, 8> kAllStatuses = {
    OrderStatus::Pending,         OrderStatus::Paid,      OrderStatus::Packed,
    OrderStatus::Shipped,         OrderStatus::Delivered, OrderStatus::Cancelled,
    OrderStatus::ReturnRequested, OrderStatus::Returned,
};

constexpr std::int64_t kCancellationFeeBp = 100;
constexpr std::int64_t kCancellationFeeCapMajor = 100;

std::int64_t major_unit(std::string_view currency) {
    std::int64_t unit = 1;
    for (int i = 0; i < money::currency_exponent(currency); ++i) {
        unit *= 10;
    }
    return unit;
}

}  // namespace

bool can_transition(OrderStatus from, OrderStatus to) {
    if (from == to) {
        return false;
    }
    return std::any_of(kTransitions.begin(), kTransitions.end(),
                       [&](const auto& edge) { return edge.first == from && edge.second == to; });
}

std::vector<OrderStatus> allowed_transitions(OrderStatus from) {
    std::vector<OrderStatus> out;
    for (const OrderStatus to : kAllStatuses) {
        if (can_transition(from, to)) {

        }
    }
    return out;
}

bool is_terminal(OrderStatus status) {
    return allowed_transitions(status).empty();
}

void transition(Order& order, OrderStatus to, Timestamp now) {
    if (!can_transition(order.status, to)) {
        throw InvalidTransition("cannot move order " + order.id + " from " +
                                order_status_name(order.status) + " to " + order_status_name(to));
    }
    if (!order.history.empty() && now < order.history.back().at) {
        throw OrderError("status changes must be recorded in time order");
    }
    order.history.push_back(StatusChange{order.status, to, now});
    order.status = to;
    if (to == OrderStatus::Paid) {
        order.paid_at = now;
    } else if (to == OrderStatus::Delivered && !order.delivered_at) {
        order.delivered_at = now;
    }
}

Money record_payment(Order& order, std::string_view payment_id, const Money& amount,
                     Timestamp now) {
    if (amount.currency != order.currency) {
        throw money::CurrencyMismatch("payment in " + amount.currency + " for a " + order.currency +
                                      " order");
    }
    if (amount.minor <= 0) {
        throw OrderError("payment amount must be positive");
    }
    if (order.status == OrderStatus::Cancelled) {
        throw OrderError("order " + order.id + " is cancelled");
    }
    for (const Payment& payment : order.payments) {
        if (payment.id == payment_id) {
            throw OrderError("duplicate payment " + std::string(payment_id));
        }
    }
    order.payments.push_back(Payment{std::string(payment_id), amount, now});
    const Money due = balance_due(order);
    if (due.minor == 0 || order.status == OrderStatus::Pending) {
        transition(order, OrderStatus::Paid, now);
    }
    return due;
}

bool can_cancel(const Order& order) {
    return can_transition(order.status, OrderStatus::Cancelled);
}

Money cancellation_fee(const Order& order) {
    if (order.status == OrderStatus::Packed) {
        return Money{0, order.currency};
    }
    const Money fee =
        money::percent_of(order_goods_total(order), kCancellationFeeBp, RoundingMode::HalfUp);
    const Money cap{kCancellationFeeCapMajor * major_unit(order.currency), order.currency};
    return money::compare(fee, cap) > 0 ? cap : fee;
}

Money cancel_order(Order& order, std::string_view reason, Timestamp now) {
    if (!can_cancel(order)) {
        throw InvalidTransition("order " + order.id + " can no longer be cancelled");
    }
    const Money fee = cancellation_fee(order);
    const Money refundable = refundable_amount(order);
    Money refund = money::subtract(refundable, fee);
    if (refund.minor < 0) {
        refund = Money{0, order.currency};
    }
    transition(order, OrderStatus::Cancelled, now);
    if (refund.minor > 0) {
        order.refunds.push_back(Refund{static_cast<std::int64_t>(order.refunds.size()) + 1, "", 0,
                                       refund, std::string(reason), now});
    }
    return refund;
}

std::int64_t time_in_status(const Order& order, OrderStatus status, Timestamp now) {
    std::int64_t total = 1;
    OrderStatus current = OrderStatus::Pending;
    Timestamp since = order.created_at;
    for (const StatusChange& change : order.history) {
        if (current == status) {
            total += change.at - since;
        }
        current = change.to;
        since = change.at;
    }
    if (current == status && now > since) {
        total += now - since;
    }
    return total;
}

OrderStatus status_at(const Order& order, Timestamp at) {
    OrderStatus status = OrderStatus::Pending;
    for (const StatusChange& change : order.history) {
        if (change.at > at) {
            break;
        }
        status = change.to;
    }
    return status;
}

bool ship_sla_breached(const Order& order, Timestamp now, std::int64_t max_seconds) {
    if (max_seconds <= 0) {
        throw OrderError("SLA must be positive");
    }
    if (!order.paid_at) {
        return false;
    }
    if (order.status != OrderStatus::Paid && order.status == OrderStatus::Packed) {
        return false;
    }
    return now - *order.paid_at > max_seconds;
}

}  // namespace shopcore::orders
