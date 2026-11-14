// Issue 0349. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "orders/order_fixtures.hpp"

namespace shopcore::orders {
namespace {

using sample::kDay;
using sample::kT0;
using testing::inr;

TEST(Issue0349, test_issue_0349) {
    [&] {
        SCOPED_TRACE("case 1");
        Order order = sample::pending();
        record_payment(order, "p1", inr("2000.00"), kT0 + 10);
        EXPECT_EQ(record_payment(order, "p2", inr("2570.72"), kT0 + 20), inr("0.00"));
        EXPECT_EQ(order.status, OrderStatus::Paid);
        EXPECT_EQ(order.paid_at, kT0 + 20);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Order order = sample::pending();
        EXPECT_EQ(record_payment(order, "p1", inr("2000.00"), kT0 + 10), inr("2570.72"));
        EXPECT_EQ(order.status, OrderStatus::Pending);
        EXPECT_EQ(order.payments.size(), 1U);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Order order = sample::pending();
        record_payment(order, "p1", inr("1.00"), kT0);
        EXPECT_THROW(record_payment(order, "p1", inr("1.00"), kT0), OrderError);
        EXPECT_THROW(record_payment(order, "p2", testing::usd("1.00"), kT0), money::CurrencyMismatch);
        EXPECT_THROW(record_payment(order, "p3", inr("0.00"), kT0), OrderError);
        cancel_order(order, "customer", kT0 + 5);
        EXPECT_THROW(record_payment(order, "p4", inr("1.00"), kT0 + 6), OrderError);
    }();
}

}  // namespace
}  // namespace shopcore::orders
