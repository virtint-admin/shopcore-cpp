// Sample ledgers shared by the ledger test suites.
#pragma once

#include "shopcore/ledger/ledger.hpp"
#include "support/fixtures.hpp"

namespace shopcore::ledger::sample {

// Sale pay_N1a2b3c on 11-02 (1678.95 gross), its settlement on 11-02, and sale
// pay_N4d5e6f on 11-03 (2948.82 gross).
inline Ledger small() {
    using testing::inr;
    Ledger ledger;
    record_sale(ledger, Date{2026, 11, 2}, "pay_N1a2b3c", inr("1400.00"), inr("229.95"),
                inr("49.00"));
    record_settlement(ledger, testing::seed_settlements()[0], "pay_N1a2b3c");
    record_sale(ledger, Date{2026, 11, 3}, "pay_N4d5e6f", inr("2499.00"), inr("449.82"),
                inr("0.00"));
    return ledger;
}

// Sales booked but not yet settled, for reconciliation against data/settlements.csv.
inline Ledger unsettled() {
    using testing::inr;
    Ledger ledger;
    record_sale(ledger, Date{2026, 11, 1}, "pay_N1a2b3c", inr("1400.00"), inr("229.95"),
                inr("49.00"));
    record_sale(ledger, Date{2026, 11, 1}, "pay_N4d5e6f", inr("2499.00"), inr("449.82"),
                inr("0.00"));
    record_sale(ledger, Date{2026, 11, 2}, "pay_N7g8h9i", inr("503.81"), inr("25.19"), inr("0.00"));
    record_sale(ledger, Date{2026, 11, 2}, "pay_Nj1k2l3", inr("1000.00"), inr("100.00"),
                inr("0.00"));
    record_sale(ledger, Date{2026, 11, 4}, "pay_UNSETTLED", inr("500.00"), inr("0.00"),
                inr("0.00"));
    record_sale(ledger, Date{2026, 11, 5}, "pay_LATE", inr("10.00"), inr("0.00"), inr("0.00"));
    return ledger;
}

}  // namespace shopcore::ledger::sample
