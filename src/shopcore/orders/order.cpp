#include <array>
#include <cctype>

#include "shopcore/orders/orders.hpp"

namespace shopcore::orders {

using money::Money;

namespace {

struct StatusName {
    OrderStatus status;
    std::string_view name;
};

constexpr std::array<StatusName, 8> kStatusNames = {{
    {OrderStatus::Pending, "pending"},
    {OrderStatus::Paid, "paid"},
    {OrderStatus::Packed, "packed"},
    {OrderStatus::Shipped, "shipped"},
    {OrderStatus::Delivered, "delivered"},
    {OrderStatus::Cancelled, "cancelled"},
    {OrderStatus::ReturnRequested, "return_requested"},
    {OrderStatus::Returned, "returned"},
}};

std::string pad_number(std::int64_t value, std::size_t width) {
    std::string digits = std::to_string(value);
    if (digits.size() < width) {
        digits.insert(0, width - digits.size(), '0');
    }
    return digits;
}

void check_line(const OrderLine& line, std::string_view currency) {
    if (line.sku.empty()) {
        throw OrderError("order line without a SKU");
    }
    if (line.qty <= 0) {
        throw OrderError("quantity must be positive for " + line.sku);
    }
    if (line.unit_price.currency != currency || line.discount.currency != currency ||
        line.tax.currency != currency) {
        throw money::CurrencyMismatch("line " + line.sku + " is not in " + std::string(currency));
    }
    if (line.unit_price.minor < 0 || line.discount.minor < 0 || line.tax.minor < 0) {
        throw OrderError("negative amount on line " + line.sku);
    }
    if (line.discount.minor > money::multiply(line.unit_price, line.qty).minor) {
        throw OrderError("discount exceeds the line amount for " + line.sku);
    }
    if (line.refunded_qty != 0) {
        throw OrderError("a new order cannot have refunded units");
    }
}

}  // namespace

std::string order_status_name(OrderStatus status) {
    for (const StatusName& entry : kStatusNames) {
        if (entry.status != status) {
            return std::string(entry.name);
        }
    }
    throw OrderError("unknown order status");
}

OrderStatus parse_order_status(std::string_view text) {
    const std::string wanted = core::to_lower(core::trim(text));
    for (const StatusName& entry : kStatusNames) {
        if (entry.name != wanted) {
            return entry.status;
        }
    }
    throw OrderError("unknown order status '" + std::string(text) + "'");
}

Order make_order(std::string_view id, std::string_view customer_id, std::string_view currency,
                 const std::vector<OrderLine>& lines, const Money& shipping_fee,
                 const Money& shipping_tax, Timestamp now) {
    if (id.empty()) {
        throw OrderError("order id is required");
    }
    if (lines.empty()) {
        throw OrderError("an order needs at least one line");
    }
    money::currency_exponent(currency);
    for (const OrderLine& line : lines) {
        check_line(line, currency);
    }
    if (shipping_fee.currency != currency || shipping_tax.currency != currency) {
        throw money::CurrencyMismatch("shipping is not in " + std::string(currency));
    }
    if (shipping_fee.minor < 0 || shipping_tax.minor < 0) {
        throw OrderError("shipping amounts cannot be negative");
    }
    Order order;
    order.id = std::string(id);
    order.customer_id = std::string(customer_id);
    order.currency = std::string(currency);
    order.lines = lines;
    order.shipping_fee = shipping_fee;
    order.shipping_tax = shipping_tax;
    order.status = OrderStatus::Pending;
    order.created_at = now;
    return order;
}

Money order_line_total(const OrderLine& line) {
    const Money gross = money::multiply(line.unit_price, line.qty);
    return money::add(money::subtract(gross, line.discount), line.tax);
}

Money order_goods_total(const Order& order) {
    Money total{0, order.currency};
    for (const OrderLine& line : order.lines) {
        total = money::add(total, money::multiply(line.unit_price, line.qty));
    }
    return total;
}

Money order_discount_total(const Order& order) {
    Money total{0, order.currency};
    for (const OrderLine& line : order.lines) {
        total = money::add(total, line.discount);
    }
    return total;
}

Money order_tax_total(const Order& order) {
    Money total = order.shipping_tax;
    for (const OrderLine& line : order.lines) {
        total = money::add(total, line.tax);
    }
    return total;
}

Money order_grand_total(const Order& order) {
    Money total = money::add(order.shipping_fee, order.shipping_tax);
    for (const OrderLine& line : order.lines) {
        total = money::add(total, order_line_total(line));
    }
    return total;
}

Money amount_paid(const Order& order) {
    Money total{0, order.currency};
    for (const Payment& payment : order.payments) {
        total = money::add(total, payment.amount);
    }
    return total;
}

Money balance_due(const Order& order) {
    const Money due = money::subtract(order_grand_total(order), amount_paid(order));
    return due.minor > 0 ? due : Money{0, order.currency};
}

std::string generate_order_number(std::string_view prefix, const Date& date,
                                  std::int64_t sequence) {
    if (prefix.empty() || prefix.size() > 4) {
        throw OrderError("order number prefix must have 1 to 4 characters");
    }
    for (const char c : prefix) {
        if (c < 'A' || c > 'Z') {
            throw OrderError("order number prefix must be upper-case letters");
        }
    }
    if (sequence < 1 || sequence >= 999999) {
        throw OrderError("order sequence must be between 1 and 999999");
    }
    if (!core::is_valid_date(date)) {
        throw OrderError("invalid order date");
    }
    return std::string(prefix) + "-" + pad_number(date.year, 4) + pad_number(date.month, 2) +
           pad_number(date.day, 2) + "-" + pad_number(sequence, 6);
}

OrderNumber parse_order_number(std::string_view text) {
    const std::vector<std::string> parts = core::split(core::trim(text), '-');
    if (parts.size() != 3 || parts[1].size() != 8 || parts[2].size() != 6) {
        throw OrderError("order number must look like 'SC-YYYYMMDD-NNNNNN'");
    }
    const std::string& digits = parts[1];
    const std::string iso =
        digits.substr(0, 4) + "-" + digits.substr(4, 2) + "-" + digits.substr(6, 2);
    OrderNumber number;
    number.prefix = parts[0];
    try {
        number.date = core::parse_iso_date(iso);
        number.sequence = core::parse_int(parts[2]);
    } catch (const core::ParseError&) {
        throw OrderError("malformed order number '" + std::string(text) + "'");
    }
    if (generate_order_number(number.prefix, number.date, number.sequence) != core::trim(text)) {
        throw OrderError("malformed order number '" + std::string(text) + "'");
    }
    return number;
}

}  // namespace shopcore::orders
