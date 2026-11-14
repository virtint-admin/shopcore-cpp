#include "shopcore/core/core.hpp"

#include <algorithm>
#include <cctype>
#include <limits>

namespace shopcore::core {

namespace {

bool is_leap(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

int days_in_month(int year, int month) {
    static constexpr int kDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && is_leap(year)) {
        return 29;
    }
    return kDays[month - 1];
}

int parse_fixed_digits(std::string_view text, std::size_t pos, std::size_t count,
                       std::string_view whole) {
    int value = 0;
    for (std::size_t i = pos; i < pos + count; ++i) {
        const char c = text[i];
        if (std::isdigit(static_cast<unsigned char>(c)) == 0) {
            throw ParseError("invalid date/time: '" + std::string(whole) + "'");
        }
        value = value * 10 + (c - '0');
    }
    return value;
}

}  // namespace

std::int64_t floor_div(std::int64_t numerator, std::int64_t denominator) {
    std::int64_t quotient = numerator / denominator;
    if ((numerator % denominator != 0) && ((numerator < 0) != (denominator < 0))) {
        --quotient;
    }
    return quotient;
}

std::int64_t days_from_civil(const Date& date) {
    const std::int64_t y = static_cast<std::int64_t>(date.year) - (date.month <= 2 ? 1 : 0);
    const std::int64_t era = floor_div(y, 400);
    const std::int64_t yoe = y - era * 400;
    const std::int64_t mp = (date.month + 9) % 12;
    const std::int64_t doy = (153 * mp + 2) / 5 + date.day - 1;
    const std::int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

Date civil_from_days(std::int64_t days) {
    const std::int64_t z = days + 719468;
    const std::int64_t era = floor_div(z, 146097);
    const std::int64_t doe = z - era * 146097;
    const std::int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const std::int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const std::int64_t mp = (5 * doy + 2) / 153;
    const std::int64_t d = doy - (153 * mp + 2) / 5 + 1;
    const std::int64_t m = mp < 10 ? mp + 3 : mp - 9;
    const std::int64_t y = yoe + era * 400 + (m <= 2 ? 1 : 0);
    return Date{static_cast<int>(y), static_cast<int>(m), static_cast<int>(d)};
}

bool is_valid_date(const Date& date) {
    if (date.month < 1 || date.month > 12 || date.day < 1) {
        return false;
    }
    return date.day <= days_in_month(date.year, date.month);
}

Date add_days(const Date& date, std::int64_t days) {
    return civil_from_days(days_from_civil(date) + days);
}

std::int64_t days_between(const Date& from, const Date& to) {
    return days_from_civil(to) - days_from_civil(from);
}

Date parse_iso_date(std::string_view text) {
    const std::string_view t = text;
    if (t.size() != 10 || t[4] != '-' || t[7] != '-') {
        throw ParseError("invalid date: '" + std::string(text) + "'");
    }
    const Date date{parse_fixed_digits(t, 0, 4, text), parse_fixed_digits(t, 5, 2, text),
                    parse_fixed_digits(t, 8, 2, text)};
    if (!is_valid_date(date)) {
        throw ParseError("invalid date: '" + std::string(text) + "'");
    }
    return date;
}

std::string format_iso_date(const Date& date) {
    std::string out(10, '0');
    int year = date.year;
    for (int i = 3; i >= 0; --i) {
        out[static_cast<std::size_t>(i)] = static_cast<char>('0' + year % 10);
        year /= 10;
    }
    out[4] = '-';
    out[5] = static_cast<char>('0' + date.month / 10);
    out[6] = static_cast<char>('0' + date.month % 10);
    out[7] = '-';
    out[8] = static_cast<char>('0' + date.day / 10);
    out[9] = static_cast<char>('0' + date.day % 10);
    return out;
}

Timestamp parse_iso_timestamp(std::string_view text) {
    // YYYY-MM-DDTHH:MM:SS followed by Z or +HH:MM / -HH:MM
    if (text.size() < 20 || text[10] != 'T' || text[13] != ':' || text[16] != ':') {
        throw ParseError("invalid timestamp: '" + std::string(text) + "'");
    }
    const Date date = parse_iso_date(text.substr(0, 10));
    const int hour = parse_fixed_digits(text, 11, 2, text);
    const int minute = parse_fixed_digits(text, 14, 2, text);
    const int second = parse_fixed_digits(text, 17, 2, text);
    if (hour > 23 || minute > 59 || second > 59) {
        throw ParseError("invalid timestamp: '" + std::string(text) + "'");
    }
    std::int64_t offset_minutes = 0;
    const std::string_view zone = text.substr(19);
    if (zone == "Z") {
        offset_minutes = 0;
    } else if (zone.size() == 6 && (zone[0] == '+' || zone[0] == '-') && zone[3] == ':') {
        const int oh = parse_fixed_digits(zone, 1, 2, text);
        const int om = parse_fixed_digits(zone, 4, 2, text);
        offset_minutes = static_cast<std::int64_t>(oh) * 60 + om;
        if (zone[0] == '-') {
            offset_minutes = -offset_minutes;
        }
    } else {
        throw ParseError("invalid timestamp zone: '" + std::string(text) + "'");
    }
    const std::int64_t local =
        start_of_day_utc(date) + hour * kSecondsPerHour + minute * kSecondsPerMinute + second;
    return local - offset_minutes * kSecondsPerMinute;
}

Timestamp start_of_day_utc(const Date& date) {
    return days_from_civil(date) * kSecondsPerDay;
}

std::string trim(std::string_view text) {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    return std::string(text.substr(begin, end - begin));
}

std::string to_upper(std::string_view text) {
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return out;
}

std::string to_lower(std::string_view text) {
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

std::vector<std::string> split(std::string_view text, char separator) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (true) {
        const std::size_t pos = text.find(separator, start);
        if (pos == std::string_view::npos) {
            parts.emplace_back(text.substr(start));
            break;
        }
        parts.emplace_back(text.substr(start, pos - start));
        start = pos + 1;
    }
    return parts;
}

bool starts_with_path(std::string_view path, std::string_view prefix) {
    if (prefix.empty()) {
        return true;
    }
    if (path.size() < prefix.size() || path.substr(0, prefix.size()) != prefix) {
        return false;
    }
    return path.size() == prefix.size() || path[prefix.size()] == '/';
}

std::int64_t parse_int(std::string_view text) {
    const std::string t = trim(text);
    if (t.empty()) {
        throw ParseError("expected an integer, got ''");
    }
    std::size_t i = 0;
    bool negative = false;
    if (t[0] == '-' || t[0] == '+') {
        negative = t[0] == '-';
        i = 1;
    }
    if (i == t.size()) {
        throw ParseError("expected an integer, got '" + t + "'");
    }
    std::int64_t value = 0;
    for (; i < t.size(); ++i) {
        if (std::isdigit(static_cast<unsigned char>(t[i])) == 0) {
            throw ParseError("expected an integer, got '" + t + "'");
        }
        if (value > (std::numeric_limits<std::int64_t>::max() - 9) / 10) {
            throw ParseError("integer out of range: '" + t + "'");
        }
        value = value * 10 + (t[i] - '0');
    }
    return negative ? -value : value;
}

bool parse_bool(std::string_view text) {
    const std::string t = to_lower(trim(text));
    if (t == "true" || t == "1" || t == "yes") {
        return true;
    }
    if (t == "false" || t == "0" || t == "no") {
        return false;
    }
    throw ParseError("expected a boolean, got '" + t + "'");
}

std::vector<std::vector<std::string>> parse_csv(std::string_view text) {
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> row;
    std::string field;
    bool in_quotes = false;
    bool field_started = false;
    auto end_row = [&]() {
        if (field_started || !row.empty()) {
            row.push_back(field);
            rows.push_back(row);
        }
        row.clear();
        field.clear();
        field_started = false;
    };
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < text.size() && text[i + 1] == '"') {
                    field.push_back('"');
                    ++i;
                } else {
                    in_quotes = false;
                }
            } else {
                field.push_back(c);
            }
            continue;
        }
        if (c == '"') {
            in_quotes = true;
            field_started = true;
        } else if (c == ',') {
            row.push_back(field);
            field.clear();
            field_started = true;
        } else if (c == '\n') {
            end_row();
        } else if (c != '\r') {
            field.push_back(c);
            field_started = true;
        }
    }
    if (in_quotes) {
        throw ParseError("unterminated quoted field in CSV");
    }
    end_row();
    return rows;
}

std::vector<std::vector<std::string>> read_table(std::string_view text,
                                                 const std::vector<std::string>& expected) {
    std::vector<std::vector<std::string>> rows = parse_csv(text);
    if (rows.empty()) {
        throw ParseError("CSV is empty; expected a header row");
    }
    std::vector<std::string> header;
    header.reserve(rows.front().size());
    for (const std::string& name : rows.front()) {
        header.push_back(trim(name));
    }
    if (header != expected) {
        throw ParseError("unexpected CSV header");
    }
    rows.erase(rows.begin());
    for (const auto& row : rows) {
        if (row.size() != expected.size()) {
            throw ParseError("CSV row has " + std::to_string(row.size()) + " fields, expected " +
                             std::to_string(expected.size()));
        }
    }
    return rows;
}

}  // namespace shopcore::core
