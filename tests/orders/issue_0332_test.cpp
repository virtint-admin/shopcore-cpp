// Issue 0332. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "orders/order_fixtures.hpp"

namespace shopcore::orders {
namespace {

using money::Money;
using sample::kT0;
using testing::inr;

TEST(Issue0332, test_issue_0332) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(parse_order_status(" Return_Requested "), OrderStatus::ReturnRequested);
        EXPECT_EQ(parse_order_status("PAID"), OrderStatus::Paid);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(parse_order_status("shipped"), OrderStatus::Shipped);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(parse_order_status("lost"), OrderError);
    }();
}

}  // namespace
}  // namespace shopcore::orders
