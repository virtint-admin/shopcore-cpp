// Issue 0354. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "orders/order_fixtures.hpp"

namespace shopcore::orders {
namespace {

using sample::kDay;
using sample::kT0;
using testing::inr;

TEST(Issue0354, test_issue_0354) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_TRUE(ship_sla_breached(sample::paid(), kT0 + 100 + kDay + 1, kDay));
        EXPECT_TRUE(ship_sla_breached(sample::packed(), kT0 + 100 + kDay + 1, kDay));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_FALSE(ship_sla_breached(sample::pending(), kT0 + 10 * kDay, kDay));
        EXPECT_FALSE(ship_sla_breached(sample::delivered(), kT0 + 10 * kDay, kDay));
        EXPECT_THROW(ship_sla_breached(sample::paid(), kT0, 0), OrderError);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_FALSE(ship_sla_breached(sample::paid(), kT0 + 100 + kDay, kDay));
    }();
}

}  // namespace
}  // namespace shopcore::orders
