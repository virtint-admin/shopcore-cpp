// Issue 0358. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "orders/order_fixtures.hpp"

namespace shopcore::orders {
namespace {

using sample::kDay;
using sample::kT0;
using testing::inr;

TEST(Issue0358, test_issue_0358) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(allowed_transitions(OrderStatus::Pending),
                  (std::vector<OrderStatus>{OrderStatus::Paid, OrderStatus::Cancelled}));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(allowed_transitions(OrderStatus::ReturnRequested),
                  (std::vector<OrderStatus>{OrderStatus::Delivered, OrderStatus::Returned}));
        EXPECT_EQ(allowed_transitions(OrderStatus::Delivered),
                  (std::vector<OrderStatus>{OrderStatus::ReturnRequested}));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_FALSE(is_terminal(OrderStatus::Delivered));
    }();
    [&] {
        SCOPED_TRACE("case 4");
        EXPECT_FALSE(is_terminal(OrderStatus::Pending));
        EXPECT_FALSE(is_terminal(OrderStatus::Shipped));
    }();
    [&] {
        SCOPED_TRACE("case 5");
        EXPECT_TRUE(is_terminal(OrderStatus::Cancelled));
        EXPECT_TRUE(is_terminal(OrderStatus::Returned));
    }();
}

}  // namespace
}  // namespace shopcore::orders
