#include <cctype>
#include <optional>

#include "shopcore/money/money.hpp"

namespace shopcore::money {

namespace {

constexpr std::string_view kPivotCurrency = "USD";

std::int64_t pow10(int exponent) {
    std::int64_t value = 1;
    for (int i = 0; i < exponent; ++i) {
        value *= 10;
    }
    return value;
}

std::int64_t invert_rate(std::int64_t rate_e6) {
    return round_div(kRateScale * kRateScale, rate_e6, RoundingMode::HalfEven);
}

std::optional<FxRate> direct_or_inverse(const std::vector<FxRate>& rates, std::string_view from,
                                        std::string_view to) {
    for (const FxRate& rate : rates) {
        if (rate.base == from && rate.quote == to) {
            return rate;
        }
    }
    for (const FxRate& rate : rates) {
        if (rate.base == to && rate.quote == from) {
            return FxRate{std::string(from), std::string(to), invert_rate(rate.rate_e6)};
        }
    }
    return std::nullopt;
}

}  // namespace

FxRate parse_fx_rate(std::string_view text) {
    const std::string t = core::trim(text);
    const std::size_t slash = t.find('/');
    const std::size_t equals = t.find('=');
    if (slash == std::string::npos || equals == std::string::npos || equals < slash) {
        throw MoneyError("expected 'BASE/QUOTE=rate', got '" + t + "'");
    }
    const std::string base = t.substr(0, slash);
    const std::string quote = t.substr(slash + 1, equals - slash);
    currency_exponent(base);
    currency_exponent(quote);
    const std::string number = t.substr(equals + 1);

    std::int64_t whole = 0;
    std::int64_t fraction = 0;
    int digits = 0;
    bool seen_point = false;
    bool seen_digit = false;
    for (const char c : number) {
        if (c == '.' && !seen_point) {
            seen_point = true;
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(c)) == 0) {
            throw MoneyError("invalid FX rate '" + number + "'");
        }
        seen_digit = true;
        if (seen_point) {
            if (++digits > 6) {
                throw MoneyError("FX rate has more than 6 decimal places: '" + number + "'");
            }
            fraction = fraction * 10 + (c - '0');
        } else {
            whole = whole * 10 + (c - '0');
        }
    }
    const std::int64_t rate_e6 = whole * kRateScale + fraction * pow10(6 - digits);
    if (!seen_digit || rate_e6 <= 0) {
        throw MoneyError("FX rate must be positive: '" + number + "'");
    }
    return FxRate{base, quote, rate_e6};
}

Money convert(const Money& amount, const FxRate& rate, RoundingMode mode) {
    if (rate.rate_e6 <= 0) {
        throw MoneyError("FX rate must be positive");
    }
    if (rate.base == rate.quote) {
        return amount;
    }
    const int base_exp = currency_exponent(rate.base);
    const int quote_exp = currency_exponent(rate.quote);
    if (amount.currency == rate.base) {
        const Money scaled = multiply_ratio(amount, rate.rate_e6 * pow10(quote_exp),
                                            kRateScale * pow10(base_exp), mode);
        return Money{scaled.minor, rate.quote};
    }
    if (amount.currency == rate.quote) {
        const Money scaled = multiply_ratio(amount, kRateScale * pow10(base_exp),
                                            rate.rate_e6 * pow10(quote_exp), mode);
        return Money{scaled.minor, rate.base};
    }
    throw CurrencyMismatch("rate " + rate.base + "/" + rate.quote + " cannot convert " +
                           amount.currency);
}

FxRate find_rate(const std::vector<FxRate>& rates, std::string_view from, std::string_view to) {
    if (from == to) {
        return FxRate{std::string(from), std::string(to), kRateScale};
    }
    if (auto direct = direct_or_inverse(rates, from, to)) {
        return *direct;
    }
    if (from != kPivotCurrency && to != kPivotCurrency) {
        const auto leg1 = direct_or_inverse(rates, from, kPivotCurrency);
        const auto leg2 = direct_or_inverse(rates, kPivotCurrency, to);
        if (leg1 && leg2) {
            return FxRate{std::string(from), std::string(to),
                          multiply_ratio(Money{leg1->rate_e6, "USD"}, leg2->rate_e6, kRateScale,
                                         RoundingMode::HalfEven)
                              .minor};
        }
    }
    throw FxRateNotFound("no FX rate from " + std::string(from) + " to " + std::string(to));
}

std::ostream& operator<<(std::ostream& out, const FxRate& rate) {
    return out << rate.base << "/" << rate.quote << "=" << rate.rate_e6 << "e-6";
}

}  // namespace shopcore::money
