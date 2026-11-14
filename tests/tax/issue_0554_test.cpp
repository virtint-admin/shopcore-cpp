// Issue 0554. Make this test pass by fixing the application code, not the test.
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

TEST(Issue0554, test_issue_0554) {
    [&] {
        SCOPED_TRACE("case 1");
        EXPECT_EQ(invoice_tax_total({inr("0.10"), inr("0.10"), inr("0.10")}, 500, true), inr("0.03"));
    }();
    [&] {
        SCOPED_TRACE("case 2");
        EXPECT_EQ(tax_exclusive(inr("0.10"), 500), inr("0.01"));
        EXPECT_EQ(tax_exclusive(inr("0.25"), 500), inr("0.01"));
    }();
    [&] {
        SCOPED_TRACE("case 3");
        EXPECT_THROW(invoice_tax_total({}, 500, true), TaxError);
    }();
    [&] {
        SCOPED_TRACE("case 4");
        EXPECT_EQ(invoice_tax_total({inr("0.10"), inr("0.10"), inr("0.10")}, 500, false), inr("0.02"));
    }();
}

}  // namespace
}  // namespace shopcore::tax
