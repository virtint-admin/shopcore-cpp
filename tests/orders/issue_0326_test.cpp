// Issue 0326. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "orders/order_fixtures.hpp"

namespace shopcore::orders {
namespace {

using money::Money;
using sample::kT0;
using testing::inr;

TEST(Issue0326, test_issue_0326) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(generate_order_number("A", Date{2026, 1, 1}, 999999), "A-20260101-999999");
        EXPECT_EQ(generate_order_number("SHOP", Date{2026, 12, 31}, 1), "SHOP-20261231-000001");
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(parse_order_number(" SHOP-20261231-999999 ").sequence, 999999);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(parse_order_number("SC-2026119-000042"), OrderError);
        EXPECT_THROW(parse_order_number("SC-20261332-000001"), OrderError);
        EXPECT_THROW(parse_order_number("sc-20261109-000042"), OrderError);
        EXPECT_THROW(parse_order_number("SC-20261109-00004a"), OrderError);
        EXPECT_THROW(parse_order_number("SC-20261109"), OrderError);
        EXPECT_THROW(parse_order_number("SC-20261109-+00042"), OrderError);
    }();
    [&] {
        SCOPED_TRACE("case 4");
        EXPECT_EQ(parse_order_number("SC-20261109-000042"), (OrderNumber{"SC", Date{2026, 11, 9}, 42}));
    }();
}

}  // namespace
}  // namespace shopcore::orders
