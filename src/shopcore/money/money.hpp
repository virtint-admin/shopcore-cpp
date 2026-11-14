// money: fixed-point currency amounts, rounding modes and FX conversion.
//
// Every amount in ShopCore is a Money: an integer count of minor units (paise, cents)
// plus an ISO 4217 currency code. Arithmetic never mixes currencies silently.
#pragma once

#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "shopcore/core/core.hpp"

namespace shopcore::money {

class MoneyError : public DomainError {
public:
    using DomainError::DomainError;
};

class CurrencyMismatch : public MoneyError {
public:
    using MoneyError::MoneyError;
};

class UnknownCurrency : public MoneyError {
public:
    using MoneyError::MoneyError;
};

class FxRateNotFound : public MoneyError {
public:
    using MoneyError::MoneyError;
};

struct Money {
    std::int64_t minor = 0;
    std::string currency = "INR";

    bool operator==(const Money&) const = default;
};

enum class RoundingMode {
    HalfUp,    // ties away from zero (commercial rounding)
    HalfEven,  // ties to even (banker's rounding)
    HalfDown,  // ties toward zero
    Down,      // toward zero (truncate)
    Up,        // away from zero
    Floor,     // toward negative infinity
    Ceiling,   // toward positive infinity
};

// Units of `quote` currency per one unit of `base`, scaled by 1,000,000.
struct FxRate {
    std::string base;
    std::string quote;
    std::int64_t rate_e6 = 0;

    bool operator==(const FxRate&) const = default;
};

inline constexpr std::int64_t kRateScale = 1'000'000;
inline constexpr std::int64_t kBasisPoints = 10'000;

// ---- amount.cpp -------------------------------------------------------------
// Number of minor-unit digits for a supported currency (INR 2, JPY 0, KWD 3, ...).
int currency_exponent(std::string_view currency);
// Builds a Money after validating the currency code.
Money make_money(std::int64_t minor, std::string_view currency);
// Parses "1234.50 INR" (amount, one space, currency). Throws MoneyError.
Money parse_money(std::string_view text);
// Machine format: "1234.50 INR", "-0.05 USD", "500 JPY".
std::string format_money(const Money& amount);
// Customer-facing format with symbol and digit grouping; INR uses lakh grouping.
std::string format_display(const Money& amount);
Money add(const Money& a, const Money& b);
Money subtract(const Money& a, const Money& b);
Money multiply(const Money& amount, std::int64_t quantity);
Money negate(const Money& amount);
// -1, 0 or 1. Throws CurrencyMismatch.
int compare(const Money& a, const Money& b);
// Sum of amounts; an empty list sums to zero in `currency`.
Money sum(const std::vector<Money>& amounts, std::string_view currency);
// Splits an amount in proportion to weights using the largest-remainder method so the
// parts always add back up to the original amount.
std::vector<Money> allocate(const Money& amount, const std::vector<std::int64_t>& weights);
// Splits into n near-equal parts; earlier parts receive the extra minor units.
std::vector<Money> split_evenly(const Money& amount, int parts);

// ---- rounding.cpp -----------------------------------------------------------
// numerator / denominator rounded to an integer with the given mode.
std::int64_t round_div(std::int64_t numerator, std::int64_t denominator, RoundingMode mode);
// amount * numerator / denominator, rounded.
Money multiply_ratio(const Money& amount, std::int64_t numerator, std::int64_t denominator,
                     RoundingMode mode);
// basis_points/10000 of the amount, rounded.
Money percent_of(const Money& amount, std::int64_t basis_points, RoundingMode mode);
// Rounds to a multiple of `increment_minor` (cash rounding, e.g. to 0.05 or 1.00).
Money round_to_increment(const Money& amount, std::int64_t increment_minor, RoundingMode mode);

// ---- fx.cpp -----------------------------------------------------------------
// Parses "USD/INR=83.125000" (up to six decimal places).
FxRate parse_fx_rate(std::string_view text);
// Converts using a rate in either direction (base->quote or quote->base).
Money convert(const Money& amount, const FxRate& rate, RoundingMode mode);
// Finds a rate from -> to: direct, inverted, or crossed through USD.
FxRate find_rate(const std::vector<FxRate>& rates, std::string_view from, std::string_view to);

// GoogleTest / logging support: prints format_money(amount).
std::ostream& operator<<(std::ostream& out, const Money& amount);
std::ostream& operator<<(std::ostream& out, const FxRate& rate);

}  // namespace shopcore::money
