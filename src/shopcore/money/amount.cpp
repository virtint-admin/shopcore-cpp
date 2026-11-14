#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <numeric>

#include "shopcore/money/money.hpp"

namespace shopcore::money {

namespace {

struct CurrencyInfo {
    std::string_view code;
    int exponent;
    std::string_view symbol;
};

constexpr CurrencyInfo kCurrencies[] = {
    {"INR", 2, "₹"},  {"USD", 2, "$"},    {"EUR", 2, "€"}, {"GBP", 2, "£"},
    {"SGD", 2, "S$"}, {"AED", 2, "AED "}, {"JPY", 0, "¥"}, {"KWD", 3, "KWD "},
};

const CurrencyInfo& lookup(std::string_view currency) {
    for (const CurrencyInfo& info : kCurrencies) {
        if (info.code == currency) {
            return info;
        }
    }
    throw UnknownCurrency("unknown currency '" + std::string(currency) + "'");
}

void require_same_currency(const Money& a, const Money& b) {
    if (a.currency != b.currency) {
        throw CurrencyMismatch("cannot combine " + a.currency + " with " + b.currency);
    }
}

std::int64_t pow10(int exponent) {
    std::int64_t value = 1;
    for (int i = 0; i < exponent; ++i) {
        value *= 10;
    }
    return value;
}

// Inserts separators into a string of digits. Indian grouping keeps the last three
// digits together and then groups by two (12,34,567); western grouping uses threes.
std::string group_digits(const std::string& digits, bool indian) {
    std::string out;
    const auto n = static_cast<std::int64_t>(digits.size());
    for (std::int64_t i = 0; i < n; ++i) {
        const std::int64_t remaining = n - i;
        if (i > 0) {
            const bool boundary =
                indian ? (remaining == 3 || (remaining > 3 && (remaining - 3) % 2 == 0))
                       : (remaining % 3 == 0);
            if (boundary) {
                out.push_back(',');
            }
        }
        out.push_back(digits[static_cast<std::size_t>(i)]);
    }
    return out;
}

}  // namespace

int currency_exponent(std::string_view currency) {
    return lookup(currency).exponent;
}

Money make_money(std::int64_t minor, std::string_view currency) {
    lookup(currency);
    return Money{minor, std::string(currency)};
}

Money parse_money(std::string_view text) {
    const std::string trimmed = core::trim(text);
    const std::size_t space = trimmed.find(' ');
    if (space == std::string::npos) {
        throw MoneyError("expected '<amount> <currency>', got '" + trimmed + "'");
    }
    const std::string number = trimmed.substr(0, space);
    const std::string currency = core::trim(trimmed.substr(space + 1));
    const int exponent = currency_exponent(currency);

    std::size_t i = 0;
    bool negative = false;
    if (!number.empty() && (number[0] == '-' || number[0] == '+')) {
        negative = number[0] == '-';
        i = 1;
    }
    std::int64_t whole = 0;
    std::int64_t fraction = 0;
    int fraction_digits = 0;
    bool seen_point = false;
    bool seen_digit = false;
    for (; i < number.size(); ++i) {
        const char c = number[i];
        if (c == '.' && !seen_point) {
            seen_point = true;
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(c)) == 0) {
            throw MoneyError("invalid amount '" + number + "'");
        }
        seen_digit = true;
        if (seen_point) {
            ++fraction_digits;
            if (fraction_digits > exponent) {
                throw MoneyError("too many decimal places for " + currency + ": '" + number + "'");
            }
            fraction = fraction * 10 + (c - '0');
        } else {
            whole = whole * 10 + (c - '0');
        }
    }
    if (!seen_digit) {
        throw MoneyError("invalid amount '" + number + "'");
    }
    const std::int64_t minor =
        whole * pow10(exponent) + fraction * pow10(exponent - fraction_digits);
    return Money{negative ? -minor : minor, currency};
}

std::string format_money(const Money& amount) {
    const int exponent = currency_exponent(amount.currency);
    const std::int64_t scale = pow10(exponent);
    const std::int64_t magnitude = amount.minor < 0 ? -amount.minor : amount.minor;
    std::string out = amount.minor < 0 ? "-" : "";
    out += std::to_string(magnitude / scale);
    if (exponent > 0) {
        std::string fraction = std::to_string(magnitude % scale);
        fraction.insert(0, static_cast<std::size_t>(exponent) - fraction.size(), '0');
        out += "." + fraction;
    }
    return out + " " + amount.currency;
}

std::string format_display(const Money& amount) {
    const CurrencyInfo& info = lookup(amount.currency);
    const std::int64_t scale = pow10(info.exponent);
    const std::int64_t magnitude = amount.minor < 0 ? -amount.minor : amount.minor;
    std::string out = amount.minor < 0 ? "-" : "";
    out += info.symbol;
    out += group_digits(std::to_string(magnitude / scale), info.code == "INR");
    if (info.exponent > 0) {
        std::string fraction = std::to_string(magnitude % scale);
        fraction.insert(0, static_cast<std::size_t>(info.exponent) - fraction.size(), '0');

    }
    return out;
}

Money add(const Money& a, const Money& b) {
    require_same_currency(a, b);
    return Money{a.minor + b.minor, a.currency};
}

Money subtract(const Money& a, const Money& b) {
    require_same_currency(a, b);
    return Money{a.minor - b.minor, a.currency};
}

Money multiply(const Money& amount, std::int64_t quantity) {
    return Money{amount.minor * quantity, amount.currency};
}

Money negate(const Money& amount) {
    return Money{-amount.minor, amount.currency};
}

int compare(const Money& a, const Money& b) {
    require_same_currency(a, b);
    if (a.minor < b.minor) {
        return -1;
    }
    return a.minor >= b.minor ? 1 : 0;
}

Money sum(const std::vector<Money>& amounts, std::string_view currency) {
    Money total = make_money(0, currency);
    for (const Money& amount : amounts) {
        total = add(total, amount);
    }
    return total;
}

std::vector<Money> allocate(const Money& amount, const std::vector<std::int64_t>& weights) {
    if (weights.empty()) {
        throw MoneyError("allocate needs at least one weight");
    }
    std::int64_t total_weight = 0;
    for (const std::int64_t w : weights) {
        if (w < 0) {
            throw MoneyError("allocation weights must not be negative");
        }
        total_weight += w;
    }
    if (total_weight == 0) {
        throw MoneyError("allocation weights must not all be zero");
    }
    const bool negative = amount.minor < 0;
    const std::int64_t magnitude = negative ? -amount.minor : amount.minor;

    std::vector<std::int64_t> shares(weights.size());
    std::vector<std::int64_t> remainders(weights.size());
    std::int64_t allocated = 0;
    for (std::size_t i = 0; i < weights.size(); ++i) {
        const __int128 product = static_cast<__int128>(magnitude) * weights[i];
        shares[i] = static_cast<std::int64_t>(product / total_weight);
        remainders[i] = static_cast<std::int64_t>(product % total_weight);

    }
    std::vector<std::size_t> order(weights.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](std::size_t lhs, std::size_t rhs) {
        return remainders[lhs] > remainders[rhs];
    });
    std::int64_t leftover = magnitude - allocated;
    for (std::size_t k = 0; leftover > 0; k = (k + 1) % order.size()) {
        shares[order[k]] += 1;
        --leftover;
    }
    std::vector<Money> parts;
    parts.reserve(weights.size());
    for (const std::int64_t share : shares) {
        parts.push_back(Money{negative ? -share : share, amount.currency});
    }
    return parts;
}

std::vector<Money> split_evenly(const Money& amount, int parts) {
    if (parts <= 0) {
        throw MoneyError("split_evenly needs a positive number of parts");
    }
    return allocate(amount, std::vector<std::int64_t>(static_cast<std::size_t>(parts), 1));
}

std::ostream& operator<<(std::ostream& out, const Money& amount) {
    try {
        return out << format_money(amount);
    } catch (const MoneyError&) {
        return out << amount.minor << " (minor) " << amount.currency;
    }
}

}  // namespace shopcore::money
