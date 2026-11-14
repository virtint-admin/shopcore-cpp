// Issue 0010. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "orders/order_fixtures.hpp"

namespace shopcore::orders {
namespace {

using money::Money;
using sample::kT0;
using testing::inr;

TEST(Issue0010, test_issue_0010) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(order_status_name(OrderStatus::Cancelled), "cancelled");
        EXPECT_EQ(order_status_name(OrderStatus::Returned), "returned");
        EXPECT_EQ(order_status_name(OrderStatus::Packed), "packed");
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(order_status_name(OrderStatus::Pending), "pending");
        EXPECT_EQ(order_status_name(OrderStatus::Delivered), "delivered");
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(order_status_name(OrderStatus::ReturnRequested), "return_requested");
    }();
}

}  // namespace
}  // namespace shopcore::orders
