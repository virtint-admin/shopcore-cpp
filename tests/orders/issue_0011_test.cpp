// Issue 0011. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "orders/order_fixtures.hpp"

namespace shopcore::orders {
namespace {

using sample::kDay;
using sample::kT0;
using testing::inr;

TEST(Issue0011, test_issue_0011) {
    [&] {
        SCOPED_TRACE("case 1");
        Order order = sample::packed();
        EXPECT_EQ(cancel_order(order, "changed mind", kT0 + 500), inr("4529.75"));
        EXPECT_EQ(order.status, OrderStatus::Cancelled);
        ASSERT_EQ(order.refunds.size(), 1U);
        EXPECT_EQ(order.refunds[0], (Refund{1, "", 0, inr("4529.75"), "changed mind", kT0 + 500}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Order order = sample::pending();
        EXPECT_EQ(cancel_order(order, "timeout", kT0 + 500), inr("0.00"));
        EXPECT_TRUE(order.refunds.empty());
        Order tiny = sample::pending();
        tiny.status = OrderStatus::Packed;
        record_payment(tiny, "p1", inr("10.00"), kT0);
        EXPECT_EQ(cancel_order(tiny, "x", kT0 + 1), inr("0.00"));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        Order order = sample::packed();
        order.lines[1].qty = 10;
        EXPECT_EQ(cancellation_fee(order), inr("100.00"));
    }();
    [&] {
        SCOPED_TRACE("case 4");
        EXPECT_EQ(cancellation_fee(sample::paid()), inr("0.00"));
        EXPECT_EQ(cancellation_fee(sample::pending()), inr("0.00"));
    }();
    [&] {
        SCOPED_TRACE("case 5");
        EXPECT_EQ(cancellation_fee(sample::packed()), inr("40.97"));
    }();
}

}  // namespace
}  // namespace shopcore::orders
