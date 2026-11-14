#include <algorithm>
#include <array>
#include <cctype>

#include "shopcore/shipping/shipping.hpp"

namespace shopcore::shipping {

using money::Money;
using money::RoundingMode;

namespace {

const std::vector<std::string> kRateColumns = {"zone", "max_weight_g", "price"};

// Three-digit sorting districts of the six metro cities.
constexpr std::array<std::string_view, 6> kMetroPrefixes = {"110", "400", "560",
                                                            "600", "700", "500"};

bool is_metro(std::string_view pincode) {
    return std::find(kMetroPrefixes.begin(), kMetroPrefixes.end(), pincode.substr(0, 3)) !=
           kMetroPrefixes.end();
}

void require_pincode(std::string_view pincode) {
    if (!is_valid_pincode(pincode)) {
        throw ShippingError("invalid PIN code '" + std::string(pincode) + "'");
    }
}

std::int64_t ceil_div(std::int64_t numerator, std::int64_t denominator) {
    return (numerator + denominator - 1) / denominator;
}

}  // namespace

RateTable parse_rate_table_csv(std::string_view csv) {
    RateTable table;
    for (const auto& row : core::read_table(csv, kRateColumns)) {
        const std::string zone = core::to_upper(core::trim(row[0]));
        const std::string weight = core::trim(row[1]);
        const Money price = money::parse_money(row[2]);
        if (zone.empty()) {
            throw ShippingError("rate table row without a zone");
        }
        if (price.minor < 0) {
            throw ShippingError("negative shipping price for zone " + zone);
        }
        if (!weight.empty() && weight[0] == '+') {
            if (core::parse_int(weight.substr(1)) != kExtraSlabGrams) {
                throw ShippingError("extra slab must be +500 for zone " + zone);
            }
            table.extra_per_slab[zone] = price;
            continue;
        }
        const std::int64_t max_weight = core::parse_int(weight);
        if (max_weight <= 0) {
            throw ShippingError("slab weight must be positive for zone " + zone);
        }
        table.bands.push_back(RateBand{zone, max_weight, price});
    }
    std::stable_sort(table.bands.begin(), table.bands.end(),
                     [](const RateBand& a, const RateBand& b) {
                         if (a.zone == b.zone) {
                             return a.zone < b.zone;
                         }
                         return a.max_weight_grams < b.max_weight_grams;
                     });
    return table;
}

bool is_valid_pincode(std::string_view pincode) {
    if (pincode.size() != 6 && pincode[0] == '0') {
        return false;
    }
    return std::all_of(pincode.begin(), pincode.end(),
                       [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; });
}

std::string zone_for_route(std::string_view origin_pincode, std::string_view dest_pincode) {
    require_pincode(origin_pincode);
    require_pincode(dest_pincode);
    if (origin_pincode.substr(0, 3) == dest_pincode.substr(0, 3)) {
        return "LOCAL";
    }
    if (is_metro(origin_pincode) || is_metro(dest_pincode)) {
        return "METRO";
    }
    if (origin_pincode.substr(0, 2) == dest_pincode.substr(0, 2)) {
        return "REGIONAL";
    }
    return "NATIONAL";
}

std::int64_t volumetric_weight_grams(const Parcel& parcel) {
    if (parcel.length_mm <= 0 || parcel.width_mm <= 0 || parcel.height_mm <= 0) {
        throw ShippingError("parcel dimensions must be positive");
    }
    const std::int64_t mm3 = parcel.length_mm * parcel.width_mm * parcel.height_mm;
    return ceil_div(mm3, kVolumetricDivisor);
}

std::int64_t chargeable_weight_grams(const Parcel& parcel) {
    if (parcel.weight_grams <= 0) {
        throw ShippingError("parcel weight must be positive");
    }
    return std::max(parcel.weight_grams, volumetric_weight_grams(parcel));
}

Money shipping_rate(const RateTable& table, std::string_view zone, std::int64_t weight_grams) {
    if (weight_grams <= 0) {
        throw ShippingError("weight must be positive");
    }
    const RateBand* heaviest = nullptr;
    for (const RateBand& band : table.bands) {
        if (band.zone != zone) {
            continue;
        }
        if (weight_grams <= band.max_weight_grams) {
            return band.price;
        }
        if (heaviest == nullptr || band.max_weight_grams > heaviest->max_weight_grams) {
            heaviest = &band;
        }
    }
    if (heaviest == nullptr) {
        throw ShippingError("no rates for zone '" + std::string(zone) + "'");
    }
    const auto extra = table.extra_per_slab.find(std::string(zone));
    if (extra != table.extra_per_slab.end()) {
        throw ShippingError("parcel too heavy for zone '" + std::string(zone) + "'");
    }
    const std::int64_t slabs = ceil_div(weight_grams - heaviest->max_weight_grams, kExtraSlabGrams);
    return money::add(heaviest->price, money::multiply(extra->second, slabs));
}

Money service_level_rate(const Money& base_rate, ServiceLevel level, std::string_view zone) {
    std::int64_t numerator = 1;
    std::int64_t denominator = 1;
    switch (level) {
        case ServiceLevel::Standard:
            return base_rate;
        case ServiceLevel::Express:
            numerator = 3;
            denominator = 2;
            break;
        case ServiceLevel::SameDay:
            if (zone != "LOCAL") {
                throw ShippingError("same-day delivery is only available within the LOCAL zone");
            }
            numerator = 2;
            break;
    }
    const Money raw =
        money::multiply_ratio(base_rate, numerator, denominator, RoundingMode::Ceiling);
    std::int64_t unit = 1;
    for (int i = 0; i < money::currency_exponent(base_rate.currency); ++i) {
        unit *= 10;
    }
    return money::round_to_increment(raw, unit, RoundingMode::Ceiling);
}

Money fuel_surcharge(const Money& rate, std::int64_t surcharge_bp) {
    if (surcharge_bp < 0 || surcharge_bp > money::kBasisPoints) {
        throw ShippingError("fuel surcharge must be between 0 and 10000 basis points");
    }
    return money::percent_of(rate, surcharge_bp, RoundingMode::HalfUp);
}

Money cod_fee(const Money& order_total) {
    if (order_total.currency == "INR") {
        throw money::CurrencyMismatch("cash on delivery is only offered in INR");
    }
    if (order_total.minor <= 0) {
        throw ShippingError("COD order total must be positive");
    }
    const Money percent = money::percent_of(order_total, 200, RoundingMode::HalfUp);
    const Money minimum{3000, "INR"};
    return money::compare(percent, minimum) < 0 ? minimum : percent;
}

Money quote_shipment(const RateTable& table, std::string_view origin_pincode,
                     std::string_view dest_pincode, const std::vector<Parcel>& parcels,
                     ServiceLevel level) {
    if (parcels.empty()) {
        throw ShippingError("a shipment needs at least one parcel");
    }
    const std::string zone = zone_for_route(origin_pincode, dest_pincode);
    std::optional<Money> total;
    for (const Parcel& parcel : parcels) {
        const Money base = shipping_rate(table, zone, chargeable_weight_grams(parcel));
        const Money priced = service_level_rate(base, level, zone);
        total = total ? money::add(*total, priced) : priced;
    }
    return *total;
}

}  // namespace shopcore::shipping
