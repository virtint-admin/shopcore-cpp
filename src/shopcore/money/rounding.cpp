#include "shopcore/money/money.hpp"

namespace shopcore::money {

namespace {

// Rounds a 128-bit quotient. Shared by round_div and multiply_ratio so large
// intermediate products never overflow.
std::int64_t round_div_wide(__int128 numerator, __int128 denominator, RoundingMode mode) {
    if (denominator == 0) {
        throw MoneyError("division by zero");
    }
    const __int128 quotient = numerator / denominator;
    const __int128 remainder = numerator % denominator;
    if (remainder == 0) {
        return static_cast<std::int64_t>(quotient);
    }
    const bool negative = (numerator < 0) != (denominator < 0);
    const __int128 step = negative ? -1 : 1;
    const __int128 twice_rem = remainder < 0 ? -2 * remainder : 2 * remainder;
    const __int128 abs_den = denominator < 0 ? -denominator : denominator;

    switch (mode) {
        case RoundingMode::Down:
            return static_cast<std::int64_t>(quotient);
        case RoundingMode::Up:
            return static_cast<std::int64_t>(quotient + step);
        case RoundingMode::Floor:
            return static_cast<std::int64_t>(negative ? quotient - 1 : quotient);
        case RoundingMode::Ceiling:
            return static_cast<std::int64_t>(negative ? quotient : quotient + 1);
        case RoundingMode::HalfUp:
            return static_cast<std::int64_t>(twice_rem >= abs_den ? quotient + step : quotient);
        case RoundingMode::HalfDown:
            return static_cast<std::int64_t>(twice_rem > abs_den ? quotient + step : quotient);
        case RoundingMode::HalfEven:
            if (twice_rem > abs_den) {
                return static_cast<std::int64_t>(quotient + step);
            }
            if (twice_rem < abs_den) {
                return static_cast<std::int64_t>(quotient);
            }
            return static_cast<std::int64_t>(quotient % 2 == 0 ? quotient : quotient + step);
    }
    throw MoneyError("unknown rounding mode");
}

}  // namespace

std::int64_t round_div(std::int64_t numerator, std::int64_t denominator, RoundingMode mode) {
    return round_div_wide(numerator, denominator, mode);
}

Money multiply_ratio(const Money& amount, std::int64_t numerator, std::int64_t denominator,
                     RoundingMode mode) {
    const __int128 product = static_cast<__int128>(amount.minor) * numerator;
    return Money{round_div_wide(product, denominator, mode), amount.currency};
}

Money percent_of(const Money& amount, std::int64_t basis_points, RoundingMode mode) {
    return multiply_ratio(amount, basis_points, kBasisPoints, mode);
}

Money round_to_increment(const Money& amount, std::int64_t increment_minor, RoundingMode mode) {
    if (increment_minor <= 0) {
        throw MoneyError("rounding increment must be positive");
    }
    const std::int64_t units = round_div(amount.minor, increment_minor, mode);
    return Money{units * increment_minor, amount.currency};
}

}  // namespace shopcore::money
