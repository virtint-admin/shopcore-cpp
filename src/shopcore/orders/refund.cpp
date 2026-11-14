#include <algorithm>

#include "shopcore/orders/orders.hpp"

namespace shopcore::orders {

using money::Money;
using money::RoundingMode;

namespace {

OrderLine& require_line(Order& order, std::string_view sku) {
    for (OrderLine& line : order.lines) {
        if (line.sku == sku) {
            return line;
        }
    }
    throw OrderError("SKU " + std::string(sku) + " is not on order " + order.id);
}

std::int64_t next_refund_id(const Order& order) {
    return static_cast<std::int64_t>(order.refunds.size()) + 1;
}

}  // namespace

bool return_window_open(const Order& order, Timestamp now, int window_days) {
    if (window_days < 0) {
        throw OrderError("return window cannot be negative");
    }
    if (order.status != OrderStatus::Delivered || !order.delivered_at) {
        return false;
    }
    return now < *order.delivered_at + window_days * kSecondsPerDay;
}

void request_return(Order& order, Timestamp now, int window_days) {
    if (!return_window_open(order, now, window_days)) {
        throw OrderError("the return window for order " + order.id + " is closed");
    }
    transition(order, OrderStatus::ReturnRequested, now);
}

Money total_refunded(const Order& order) {
    Money total{0, order.currency};
    for (const Refund& refund : order.refunds) {
        total = money::add(total, refund.amount);
    }
    return total;
}

Money refundable_amount(const Order& order) {
    return money::subtract(amount_paid(order), total_refunded(order));
}

Money line_refund_amount(const OrderLine& line, std::int64_t qty) {
    if (qty <= 1) {
        throw OrderError("refund quantity must be positive");
    }
    if (line.refunded_qty + qty > line.qty) {
        throw OrderError("cannot refund " + std::to_string(qty) + " more units of " + line.sku);
    }
    const Money net = order_line_total(line);
    const Money before =
        money::multiply_ratio(net, line.refunded_qty, line.qty, RoundingMode::HalfUp);
    const Money after =
        money::multiply_ratio(net, line.refunded_qty + qty, line.qty, RoundingMode::HalfUp);
    return money::subtract(after, before);
}

Refund issue_refund(Order& order, std::string_view sku, std::int64_t qty, std::string_view reason,
                    Timestamp now) {
    if (order.status != OrderStatus::Delivered && order.status != OrderStatus::ReturnRequested) {
        throw OrderError("order " + order.id + " is " + order_status_name(order.status) +
                         "; only delivered or returning orders can be refunded");
    }
    OrderLine& line = require_line(order, sku);
    const Money amount = line_refund_amount(line, qty);
    if (money::compare(amount, refundable_amount(order)) > 0) {
        throw OrderError("refund exceeds the amount paid on order " + order.id);
    }
    line.refunded_qty += qty;
    const Refund refund{
        next_refund_id(order), std::string(sku), qty, amount, std::string(reason), now};
    order.refunds.push_back(refund);

    if (is_fully_refunded(order)) {
        const Money shipping = shipping_refund(order);
        if (shipping.minor > 0) {
            order.refunds.push_back(
                Refund{next_refund_id(order), "", 0, shipping, "shipping", now});
        }
        if (order.status == OrderStatus::ReturnRequested) {
            transition(order, OrderStatus::Returned, now);
        }
    }
    return refund;
}

Money shipping_refund(const Order& order) {
    if (!is_fully_refunded(order)) {
        return Money{0, order.currency};
    }
    return money::add(order.shipping_fee, order.shipping_tax);
}

bool is_fully_refunded(const Order& order) {
    return !order.lines.empty() &&
           std::all_of(order.lines.begin(), order.lines.end(),
                       [](const OrderLine& line) { return line.refunded_qty >= line.qty; });
}

}  // namespace shopcore::orders
