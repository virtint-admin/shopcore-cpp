// shipping: courier rate tables, actual vs volumetric weight, packing, and delivery date
// estimates.
//
// Zones are derived from Indian PIN codes (six digits). Rate tables come from
// data/rate_table.csv: one row per weight slab and zone, plus a "+500" row giving the
// price of every additional 500 g above the heaviest slab.
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "shopcore/core/core.hpp"
#include "shopcore/money/money.hpp"

namespace shopcore::shipping {

class ShippingError : public DomainError {
public:
    using DomainError::DomainError;
};

inline constexpr std::int64_t kExtraSlabGrams = 500;
// Courier volumetric divisor: kg = (L x W x H in cm) / 5000, i.e. grams = mm^3 / 5000.
inline constexpr std::int64_t kVolumetricDivisor = 5000;

struct RateBand {
    std::string zone;
    std::int64_t max_weight_grams = 0;
    money::Money price;

    bool operator==(const RateBand&) const = default;
};

struct RateTable {
    std::vector<RateBand> bands;                         // sorted by zone, then weight
    std::map<std::string, money::Money> extra_per_slab;  // zone -> price per extra 500 g

    bool operator==(const RateTable&) const = default;
};

struct Parcel {
    std::int64_t weight_grams = 0;
    std::int64_t length_mm = 0;
    std::int64_t width_mm = 0;
    std::int64_t height_mm = 0;

    bool operator==(const Parcel&) const = default;
};

struct Box {
    std::string name;
    std::int64_t length_mm = 0;
    std::int64_t width_mm = 0;
    std::int64_t height_mm = 0;
    std::int64_t max_weight_grams = 0;

    bool operator==(const Box&) const = default;
};

struct PackItem {
    std::string sku;
    std::int64_t weight_grams = 0;
    std::int64_t qty = 0;

    bool operator==(const PackItem&) const = default;
};

enum class ServiceLevel { Standard, Express, SameDay };

struct TransitRange {
    int min_days = 0;
    int max_days = 0;

    bool operator==(const TransitRange&) const = default;
};

struct DeliveryEstimate {
    Date dispatch;
    Date earliest;
    Date latest;

    bool operator==(const DeliveryEstimate&) const = default;
};

// ---- rates.cpp --------------------------------------------------------------
// Parses data/rate_table.csv format.
RateTable parse_rate_table_csv(std::string_view csv);
// Six digits, first digit 1-9.
bool is_valid_pincode(std::string_view pincode);
// LOCAL (same 3-digit sorting district), METRO (both in metro cities), REGIONAL (same
// postal circle, first 2 digits) or NATIONAL.
std::string zone_for_route(std::string_view origin_pincode, std::string_view dest_pincode);
// Volumetric weight in grams, rounded up.
std::int64_t volumetric_weight_grams(const Parcel& parcel);
// The larger of actual and volumetric weight.
std::int64_t chargeable_weight_grams(const Parcel& parcel);
// Price of the lightest slab that fits; above the heaviest slab every started 500 g
// costs the zone's extra-slab price.
money::Money shipping_rate(const RateTable& table, std::string_view zone,
                           std::int64_t weight_grams);
// Standard 1x, Express 1.5x, SameDay 2x (LOCAL zone only); rounded up to a whole unit.
money::Money service_level_rate(const money::Money& base_rate, ServiceLevel level,
                                std::string_view zone);
// Fuel surcharge in basis points of the rate, rounded half-up.
money::Money fuel_surcharge(const money::Money& rate, std::int64_t surcharge_bp);
// Cash-on-delivery fee: 2% of the order total, at least 30.00 INR. INR only.
money::Money cod_fee(const money::Money& order_total);
// Sum of rates for every parcel, each priced by its chargeable weight.
money::Money quote_shipment(const RateTable& table, std::string_view origin_pincode,
                            std::string_view dest_pincode, const std::vector<Parcel>& parcels,
                            ServiceLevel level);

// ---- packaging.cpp ----------------------------------------------------------
// Longest side + 2 x (sum of the other two sides).
std::int64_t length_plus_girth_mm(const Parcel& parcel);
// Over 30 kg, any side over 1500 mm or length plus girth over 3000 mm.
bool is_oversize(const Parcel& parcel);
// True if the item fits the box in some orientation and is not too heavy for it.
bool box_fits(const Parcel& item, const Box& box);
// Smallest-volume box the item fits (ties: name order).
std::optional<Box> smallest_box(const Parcel& item, const std::vector<Box>& boxes);
// Minimum number of parcels for a total weight given a per-parcel limit.
std::int64_t parcel_count(std::int64_t total_weight_grams, std::int64_t max_parcel_grams);
// First-fit-decreasing packing of unit weights; returns parcel weights in fill order.
std::vector<std::int64_t> pack_by_weight(const std::vector<PackItem>& items,
                                         std::int64_t max_parcel_grams);

// ---- delivery.cpp -----------------------------------------------------------
// 0 = Monday ... 6 = Sunday.
int weekday(const Date& date);
// Couriers work Monday to Saturday, except on holidays.
bool is_business_day(const Date& date, const std::vector<Date>& holidays);
Date add_business_days(const Date& date, int days, const std::vector<Date>& holidays);
// Business days strictly after `from` up to and including `to` (0 when to <= from).
int business_days_between(const Date& from, const Date& to, const std::vector<Date>& holidays);
TransitRange transit_days(std::string_view zone, ServiceLevel level);
// Local calendar date of a timestamp in a fixed UTC offset.
Date local_date(Timestamp at, int tz_offset_minutes);
// The order ships the same local day if placed before the cutoff hour on a business
// day, otherwise on the next business day.
Date dispatch_date(Timestamp order_time, int tz_offset_minutes, int cutoff_hour,
                   const std::vector<Date>& holidays);
DeliveryEstimate estimate_delivery(Timestamp order_time, int tz_offset_minutes,
                                   std::string_view zone, ServiceLevel level,
                                   const std::vector<Date>& holidays, int cutoff_hour);
// Delivered after the promised local date.
bool is_late(const Date& promised, Timestamp delivered_at, int tz_offset_minutes);
// "Tue 10 Nov" or "Tue 10 Nov - Thu 12 Nov".
std::string format_delivery_window(const DeliveryEstimate& estimate);

}  // namespace shopcore::shipping
