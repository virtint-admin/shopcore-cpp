// Issue 0559. Make this test pass by fixing the application code, not the test.
#include <gtest/gtest.h>

#include "shopcore/tax/tax.hpp"
#include "support/fixtures.hpp"

namespace shopcore::tax {
namespace {

using money::Money;
using testing::inr;
using testing::seed_tax_rules;

const Date kToday{2026, 11, 9};

std::vector<TaxableLine> basket() {
    return {TaxableLine{"APP-10001-BLU-M", "apparel/men/tshirts", inr("1598.00")},
            TaxableLine{"ELE-30001", "electronics/audio", inr("2499.00")}};
}

TEST(Issue0559, test_issue_0559) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(effective_rate_bp(inr("529.72"), inr("4097.00")), 1293);
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(effective_rate_bp(inr("79.90"), inr("1598.00")), 500);
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_EQ(effective_rate_bp(inr("0.00"), inr("0.00")), 0);
        EXPECT_THROW(effective_rate_bp(inr("1.00"), testing::usd("1.00")), money::CurrencyMismatch);
    }();
}

}  // namespace
}  // namespace shopcore::tax
