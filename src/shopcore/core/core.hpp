// Shared infrastructure for every ShopCore module.
//
// Nothing in here is a "business" function: these are calendar and CSV helpers that the
// domain modules build on. They live in shopcore::core and are not part of any module's
// public API (see ARCHITECTURE.md, "Core helpers").
#pragma once

#include <compare>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace shopcore {

// Base class for every error raised by ShopCore. Modules derive their own error types.
class DomainError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Seconds since 1970-01-01T00:00:00Z. Time is always injected by the caller; ShopCore
// never reads the wall clock.
using Timestamp = std::int64_t;

inline constexpr std::int64_t kSecondsPerMinute = 60;
inline constexpr std::int64_t kSecondsPerHour = 3600;
inline constexpr std::int64_t kSecondsPerDay = 86400;

// A calendar date (proleptic Gregorian), no time zone attached.
struct Date {
    int year = 1970;
    int month = 1;
    int day = 1;

    auto operator<=>(const Date&) const = default;
};

namespace core {

class ParseError : public DomainError {
public:
    using DomainError::DomainError;
};

// Days since 1970-01-01 for a civil date (Howard Hinnant's algorithm).
std::int64_t days_from_civil(const Date& date);
// Inverse of days_from_civil.
Date civil_from_days(std::int64_t days);
// True if the date exists in the Gregorian calendar.
bool is_valid_date(const Date& date);
Date add_days(const Date& date, std::int64_t days);
std::int64_t days_between(const Date& from, const Date& to);
// "2026-11-09" -> Date. Throws ParseError.
Date parse_iso_date(std::string_view text);
std::string format_iso_date(const Date& date);
// "2026-11-09T10:30:00+05:30" or "...Z" -> Timestamp. Throws ParseError.
Timestamp parse_iso_timestamp(std::string_view text);
// Start of the civil day in UTC, as a Timestamp.
Timestamp start_of_day_utc(const Date& date);
// Floor division that rounds toward negative infinity.
std::int64_t floor_div(std::int64_t numerator, std::int64_t denominator);

// String helpers.
std::string trim(std::string_view text);
std::string to_upper(std::string_view text);
std::string to_lower(std::string_view text);
std::vector<std::string> split(std::string_view text, char separator);
bool starts_with_path(std::string_view path, std::string_view prefix);

// Parses a strict decimal integer ("-12", "40"). Throws ParseError.
std::int64_t parse_int(std::string_view text);
// Parses "true"/"false"/"1"/"0"/"yes"/"no" (case-insensitive). Throws ParseError.
bool parse_bool(std::string_view text);

// RFC 4180 CSV reader. Returns rows (header included); blank lines are skipped.
std::vector<std::vector<std::string>> parse_csv(std::string_view text);

// Reads CSV text with a header row and checks the header matches `expected`.
// Returns the data rows only. Throws ParseError on a header mismatch or ragged row.
std::vector<std::vector<std::string>> read_table(std::string_view text,
                                                 const std::vector<std::string>& expected);

}  // namespace core
}  // namespace shopcore
