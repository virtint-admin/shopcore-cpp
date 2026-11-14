// Sample sale records shared by the reports test suites.
#pragma once

#include "shopcore/reports/reports.hpp"
#include "support/fixtures.hpp"

namespace shopcore::reports::sample {

// Four orders over 2026-11-02..05; net total 15041.00 INR.
inline std::vector<SaleRecord> records() {
    using testing::inr;
    return {
        {"SC-1", Date{2026, 11, 2}, "APP-10001-BLU-M", "apparel/men/tshirts", "KA", 2,
         inr("1498.00"), inr("74.90")},
        {"SC-1", Date{2026, 11, 2}, "ELE-30001", "electronics/audio", "KA", 1, inr("2499.00"),
         inr("449.82")},
        {"SC-2", Date{2026, 11, 3}, "APP-20001-RED-XL", "apparel/women/sarees", "MH", 1,
         inr("4999.00"), inr("249.95")},
        {"SC-3", Date{2026, 11, 3}, "ELE-30001", "electronics/audio", "DL", 2, inr("4998.00"),
         inr("899.64")},
        {"SC-4", Date{2026, 11, 5}, "GRO-50001", "grocery/beverages", "KA", 3, inr("1047.00"),
         inr("52.35")},
    };
}

}  // namespace shopcore::reports::sample
