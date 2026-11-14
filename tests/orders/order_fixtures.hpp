// Sample orders shared by the orders test suites.
#pragma once

#include "shopcore/orders/orders.hpp"
#include "support/fixtures.hpp"

namespace shopcore::orders::sample {

inline constexpr Timestamp kT0 = 1'794'200'000;  // order creation time
inline constexpr std::int64_t kDay = kSecondsPerDay;

// Two T-shirts (100.00 off, 5% GST) and earbuds (18% GST), 49.00 shipping.
// Goods 4097.00, discount 100.00, tax 524.72, grand total 4570.72.
inline std::vector<OrderLine> lines() {
    using testing::inr;
    return {OrderLine{"APP-10001-BLU-M", "apparel/men/tshirts", 2, inr("799.00"), inr("100.00"),
                      inr("74.90"), 0},
            OrderLine{"ELE-30001", "electronics/audio", 1, inr("2499.00"), inr("0.00"),
                      inr("449.82"), 0}};
}

inline Order pending() {
    using testing::inr;
    return make_order("SC-20261109-000042", "cust-9", "INR", lines(), inr("49.00"), inr("0.00"),
                      kT0);
}

inline Order paid() {
    Order order = pending();
    record_payment(order, "pay_1", order_grand_total(order), kT0 + 100);
    return order;
}

inline Order packed() {
    Order order = paid();
    transition(order, OrderStatus::Packed, kT0 + 400);
    return order;
}

inline Order delivered() {
    Order order = packed();
    transition(order, OrderStatus::Shipped, kT0 + kDay);
    transition(order, OrderStatus::Delivered, kT0 + 3 * kDay);
    return order;
}

inline Order returning() {
    Order order = delivered();
    request_return(order, kT0 + 4 * kDay, 7);
    return order;
}

}  // namespace shopcore::orders::sample
