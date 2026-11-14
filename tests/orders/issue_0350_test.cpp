// Issue 0350. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "orders/order_fixtures.hpp"

namespace shopcore::orders {
namespace {

using sample::kDay;
using sample::kT0;
using testing::inr;

TEST(Issue0350, test_issue_0350) {
    [&] {
        SCOPED_TRACE("case 1");
        const Order order = sample::packed();
        EXPECT_EQ(time_in_status(order, OrderStatus::Pending, kT0 + 1000), 100);
        EXPECT_EQ(time_in_status(order, OrderStatus::Paid, kT0 + 1000), 300);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(time_in_status(sample::packed(), OrderStatus::Packed, kT0 + 1000), 600);
        EXPECT_EQ(time_in_status(sample::pending(), OrderStatus::Pending, kT0 + 50), 50);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(time_in_status(sample::packed(), OrderStatus::Shipped, kT0 + 1000), 0);
        EXPECT_EQ(time_in_status(sample::pending(), OrderStatus::Pending, kT0 - 10), 0);
    }();
}

}  // namespace
}  // namespace shopcore::orders
