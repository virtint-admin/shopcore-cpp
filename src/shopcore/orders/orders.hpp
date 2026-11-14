// orders: order construction and totals, the order state machine, cancellation and
// refunds.
//
// State machine:
//
//   Pending --pay--> Paid --pack--> Packed --ship--> Shipped --deliver--> Delivered
//      |              |               |                                     |
//      +--------------+---------------+--> Cancelled          ReturnRequested
//                                                              |          |
//                                                          Returned   Delivered (rejected)
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "shopcore/core/core.hpp"
#include "shopcore/money/money.hpp"

namespace shopcore::orders {

class OrderError : public DomainError {
public:
    using DomainError::DomainError;
};

class InvalidTransition : public OrderError {
public:
    using OrderError::OrderError;
};

enum class OrderStatus {
    Pending,
    Paid,
    Packed,
    Shipped,
    Delivered,
    Cancelled,
    ReturnRequested,
    Returned,
};

struct OrderLine {
    std::string sku;
    std::string category;
    std::int64_t qty = 0;
    money::Money unit_price;
    money::Money discount;  // whole-line discount
    money::Money tax;       // whole-line tax on (unit_price x qty - discount)
    std::int64_t refunded_qty = 0;

    bool operator==(const OrderLine&) const = default;
};

struct StatusChange {
    OrderStatus from = OrderStatus::Pending;
    OrderStatus to = OrderStatus::Pending;
    Timestamp at = 0;

    bool operator==(const StatusChange&) const = default;
};

struct Payment {
    std::string id;
    money::Money amount;
    Timestamp at = 0;

    bool operator==(const Payment&) const = default;
};

struct Refund {
    std::int64_t id = 0;
    std::string sku;  // empty for an order-level refund (cancellation, shipping)
    std::int64_t qty = 0;
    money::Money amount;
    std::string reason;
    Timestamp at = 0;

    bool operator==(const Refund&) const = default;
};

struct Order {
    std::string id;
    std::string customer_id;
    std::string currency = "INR";
    std::vector<OrderLine> lines;
    money::Money shipping_fee;
    money::Money shipping_tax;
    OrderStatus status = OrderStatus::Pending;
    std::vector<StatusChange> history;
    std::vector<Payment> payments;
    std::vector<Refund> refunds;
    Timestamp created_at = 0;
    std::optional<Timestamp> paid_at;
    std::optional<Timestamp> delivered_at;

    bool operator==(const Order&) const = default;
};

struct OrderNumber {
    std::string prefix;
    Date date;
    std::int64_t sequence = 0;

    bool operator==(const OrderNumber&) const = default;
};

// ---- order.cpp --------------------------------------------------------------
// "pending", "paid", "packed", "shipped", "delivered", "cancelled", "return_requested",
// "returned".
std::string order_status_name(OrderStatus status);
OrderStatus parse_order_status(std::string_view text);
// Validates the lines (same currency, positive quantity, discount within the line) and
// creates a Pending order.
Order make_order(std::string_view id, std::string_view customer_id, std::string_view currency,
                 const std::vector<OrderLine>& lines, const money::Money& shipping_fee,
                 const money::Money& shipping_tax, Timestamp now);
// unit_price x qty - discount + tax.
money::Money order_line_total(const OrderLine& line);
// Sum of unit_price x qty over all lines (before discounts).
money::Money order_goods_total(const Order& order);
money::Money order_discount_total(const Order& order);
// Line taxes plus shipping tax.
money::Money order_tax_total(const Order& order);
// Goods - discounts + taxes + shipping.
money::Money order_grand_total(const Order& order);
money::Money amount_paid(const Order& order);
// Grand total minus payments, never below zero.
money::Money balance_due(const Order& order);
// "SC-20261109-000042".
std::string generate_order_number(std::string_view prefix, const Date& date, std::int64_t sequence);
OrderNumber parse_order_number(std::string_view text);

// ---- state.cpp --------------------------------------------------------------
bool can_transition(OrderStatus from, OrderStatus to);
// In declaration order of OrderStatus.
std::vector<OrderStatus> allowed_transitions(OrderStatus from);
bool is_terminal(OrderStatus status);
// Moves the order, records the change and stamps paid_at / delivered_at. Throws
// InvalidTransition.
void transition(Order& order, OrderStatus to, Timestamp now);
// Adds a payment; a Pending order that is now fully paid becomes Paid. Returns the
// balance still due.
money::Money record_payment(Order& order, std::string_view payment_id, const money::Money& amount,
                            Timestamp now);
bool can_cancel(const Order& order);
// Restocking fee for cancelling a packed order: 1% of the goods total, at most 100.00
// (in the order currency's major unit); zero before packing.
money::Money cancellation_fee(const Order& order);
// Cancels the order and books a refund of everything paid minus the cancellation fee.
// Returns the refund amount (zero if nothing was paid).
money::Money cancel_order(Order& order, std::string_view reason, Timestamp now);
// Seconds the order spent in `status` up to `now`.
std::int64_t time_in_status(const Order& order, OrderStatus status, Timestamp now);
// Status the order was in at time `at` (Pending before its first change).
OrderStatus status_at(const Order& order, Timestamp at);
// Paid more than `max_seconds` ago and not yet shipped.
bool ship_sla_breached(const Order& order, Timestamp now, std::int64_t max_seconds);

// ---- refund.cpp -------------------------------------------------------------
// Delivered and still inside the return window (exclusive end).
bool return_window_open(const Order& order, Timestamp now, int window_days);
// Delivered -> ReturnRequested, if the window is open.
void request_return(Order& order, Timestamp now, int window_days);
money::Money total_refunded(const Order& order);
// Payments minus refunds.
money::Money refundable_amount(const Order& order);
// Refund for `qty` more units of a line, given how many were already refunded. Amounts
// are cumulative-rounded so refunding every unit returns the exact line total.
money::Money line_refund_amount(const OrderLine& line, std::int64_t qty);
// Refunds units of a line. When every unit of every line has been refunded the shipping
// is refunded too, and a ReturnRequested order becomes Returned.
Refund issue_refund(Order& order, std::string_view sku, std::int64_t qty, std::string_view reason,
                    Timestamp now);
// Shipping fee plus shipping tax, refundable only once every unit has been refunded.
money::Money shipping_refund(const Order& order);
bool is_fully_refunded(const Order& order);

}  // namespace shopcore::orders
