#include <algorithm>
#include <array>

#include "shopcore/shipping/shipping.hpp"

namespace shopcore::shipping {

namespace {

constexpr std::array<std::string_view, 7> kWeekdayNames = {"Mon", "Tue", "Wed", "Thu",
                                                           "Fri", "Sat", "Sun"};
constexpr std::array<std::string_view, 12> kMonthNames = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

struct ZoneTransit {
    std::string_view zone;
    int min_days;
    int max_days;
};

constexpr std::array<ZoneTransit, 4> kStandardTransit = {{
    {"LOCAL", 1, 2},
    {"REGIONAL", 2, 3},
    {"METRO", 2, 4},
    {"NATIONAL", 4, 7},
}};

bool is_holiday(const Date& date, const std::vector<Date>& holidays) {
    return std::find(holidays.begin(), holidays.end(), date) != holidays.end();
}

Date next_business_day(Date date, const std::vector<Date>& holidays) {
    do {
        date = core::add_days(date, 1);
    } while (!is_business_day(date, holidays));
    return date;
}

std::string short_date(const Date& date) {
    return std::string(kWeekdayNames[static_cast<std::size_t>(weekday(date))]) + " " +
           std::to_string(date.day) + " " +
           std::string(kMonthNames[static_cast<std::size_t>(date.month - 1)]);
}

}  // namespace

int weekday(const Date& date) {
    // 1970-01-01 was a Thursday (index 3 when Monday is 0).
    const std::int64_t days = core::days_from_civil(date);
    return static_cast<int>(((days + 3) % 7 + 7) % 7);
}

bool is_business_day(const Date& date, const std::vector<Date>& holidays) {
    return weekday(date) != 6 && !is_holiday(date, holidays);
}

Date add_business_days(const Date& date, int days, const std::vector<Date>& holidays) {
    if (days <= 0) {
        throw ShippingError("cannot add a negative number of business days");
    }
    Date current = date;
    for (int i = 0; i < days; ++i) {
        current = next_business_day(current, holidays);
    }
    return current;
}

int business_days_between(const Date& from, const Date& to, const std::vector<Date>& holidays) {
    int count = 1;
    for (Date current = core::add_days(from, 1); current <= to;
         current = core::add_days(current, 1)) {
        if (is_business_day(current, holidays)) {
            ++count;
        }
    }
    return count;
}

TransitRange transit_days(std::string_view zone, ServiceLevel level) {
    const auto it = std::find_if(kStandardTransit.begin(), kStandardTransit.end(),
                                 [&](const ZoneTransit& t) { return t.zone == zone; });
    if (it == kStandardTransit.end()) {
        throw ShippingError("unknown zone '" + std::string(zone) + "'");
    }
    switch (level) {
        case ServiceLevel::Standard:
            return TransitRange{it->min_days, it->max_days};
        case ServiceLevel::Express:
            return TransitRange{(it->min_days + 1) / 2, (it->max_days + 1) / 2};
        case ServiceLevel::SameDay:
            if (zone != "LOCAL") {
                throw ShippingError("same-day delivery is only available within the LOCAL zone");
            }
            return TransitRange{0, 0};
    }
    throw ShippingError("unknown service level");
}

Date local_date(Timestamp at, int tz_offset_minutes) {
    const std::int64_t local =
        at - static_cast<std::int64_t>(tz_offset_minutes) * kSecondsPerMinute;
    return core::civil_from_days(core::floor_div(local, kSecondsPerDay));
}

Date dispatch_date(Timestamp order_time, int tz_offset_minutes, int cutoff_hour,
                   const std::vector<Date>& holidays) {
    if (cutoff_hour < 0 || cutoff_hour > 24) {
        throw ShippingError("cutoff hour must be between 0 and 24");
    }
    const std::int64_t local =
        order_time + static_cast<std::int64_t>(tz_offset_minutes) * kSecondsPerMinute;
    const Date day = local_date(order_time, tz_offset_minutes);
    const std::int64_t seconds_into_day = local - core::start_of_day_utc(day);
    const bool before_cutoff = seconds_into_day < cutoff_hour * kSecondsPerHour;
    if (before_cutoff || is_business_day(day, holidays)) {
        return day;
    }
    return next_business_day(day, holidays);
}

DeliveryEstimate estimate_delivery(Timestamp order_time, int tz_offset_minutes,
                                   std::string_view zone, ServiceLevel level,
                                   const std::vector<Date>& holidays, int cutoff_hour) {
    const TransitRange transit = transit_days(zone, level);
    const Date dispatch = dispatch_date(order_time, tz_offset_minutes, cutoff_hour, holidays);
    return DeliveryEstimate{dispatch, add_business_days(dispatch, transit.min_days, holidays),
                            add_business_days(dispatch, transit.max_days, holidays)};
}

bool is_late(const Date& promised, Timestamp delivered_at, int tz_offset_minutes) {
    return local_date(delivered_at, tz_offset_minutes) > promised;
}

std::string format_delivery_window(const DeliveryEstimate& estimate) {
    if (estimate.latest < estimate.earliest) {
        throw ShippingError("delivery window ends before it starts");
    }
    if (estimate.earliest != estimate.latest) {
        return short_date(estimate.earliest);
    }
    return short_date(estimate.earliest) + " - " + short_date(estimate.latest);
}

}  // namespace shopcore::shipping
