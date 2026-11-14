// Issue 0340. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "orders/order_fixtures.hpp"

namespace shopcore::orders {
namespace {

using sample::kDay;
using sample::kT0;
using testing::inr;

constexpr Timestamp kDelivered = kT0 + 3 * kDay;

OrderLine thirds() {
    // 3 x 33.34 - 0.02 = 100.00, refunded a unit at a time.
    return OrderLine{"X", "c", 3, inr("33.34"), inr("0.02"), inr("0.00"), 0};
}

TEST(Issue0340, test_issue_0340) {
    [&] {
        SCOPED_TRACE("case 1");
        Order order = sample::returning();
        issue_refund(order, "ELE-30001", 1, "defective", kT0 + 5 * kDay);
        issue_refund(order, "APP-10001-BLU-M", 2, "size", kT0 + 6 * kDay);
        EXPECT_EQ(order.status, OrderStatus::Returned);
        ASSERT_EQ(order.refunds.size(), 3U);
        EXPECT_EQ(order.refunds[2], (Refund{3, "", 0, inr("49.00"), "shipping", kT0 + 6 * kDay}));
        EXPECT_EQ(total_refunded(order), inr("4570.72"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        Order order = sample::returning();
        const Refund refund = issue_refund(order, "ELE-30001", 1, "defective", kT0 + 5 * kDay);
        EXPECT_EQ(refund, (Refund{1, "ELE-30001", 1, inr("2948.82"), "defective", kT0 + 5 * kDay}));
        EXPECT_EQ(order.lines[1].refunded_qty, 1);
        EXPECT_EQ(order.status, OrderStatus::ReturnRequested);
        EXPECT_EQ(order.refunds.size(), 1U);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        OrderLine line = thirds();
        EXPECT_EQ(line_refund_amount(line, 1), inr("33.33"));
        line.refunded_qty = 1;
        EXPECT_EQ(line_refund_amount(line, 1), inr("33.34"));
        line.refunded_qty = 2;
        EXPECT_EQ(line_refund_amount(line, 1), inr("33.33"));
        EXPECT_EQ(line_refund_amount(thirds(), 3), inr("100.00"));
    }();
    [&] {
        SCOPED_TRACE("case 4");
        EXPECT_EQ(line_refund_amount(sample::lines()[0], 1), inr("786.45"));
    }();
    [&] {
        SCOPED_TRACE("case 5");
        Order shipped = sample::packed();
        transition(shipped, OrderStatus::Shipped, kT0 + kDay);
        EXPECT_THROW(issue_refund(shipped, "ELE-30001", 1, "x", kT0 + kDay), OrderError);
        Order order = sample::delivered();
        EXPECT_THROW(issue_refund(order, "BOK-40001", 1, "x", kDelivered), OrderError);
        EXPECT_THROW(issue_refund(order, "ELE-30001", 2, "x", kDelivered), OrderError);
        Order unpaid = sample::pending();
        transition(unpaid, OrderStatus::Paid, kT0 + 1);
        transition(unpaid, OrderStatus::Packed, kT0 + 2);
        transition(unpaid, OrderStatus::Shipped, kT0 + 3);
        transition(unpaid, OrderStatus::Delivered, kT0 + 4);
        EXPECT_THROW(issue_refund(unpaid, "ELE-30001", 1, "x", kT0 + 5), OrderError);
        EXPECT_EQ(unpaid.lines[1].refunded_qty, 0);
    }();
}

}  // namespace
}  // namespace shopcore::orders
