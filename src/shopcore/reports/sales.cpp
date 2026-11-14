#include <algorithm>
#include <map>
#include <set>

#include "shopcore/reports/reports.hpp"

namespace shopcore::reports {

using money::Money;
using money::RoundingMode;

namespace {

Date local_business_date(Timestamp at, int tz_offset_minutes) {
    const std::int64_t local =
        at + static_cast<std::int64_t>(tz_offset_minutes) * kSecondsPerMinute;
    return core::civil_from_days(core::floor_div(local, kSecondsPerDay));
}

std::string category_prefix(std::string_view category, int depth) {
    std::string out;
    int taken = 0;
    for (const std::string& part : core::split(category, '/')) {
        if (taken == depth) {
            break;
        }
        if (!out.empty()) {
            out += "/";
        }
        out += part;
        ++taken;
    }
    return out;
}

std::vector<GroupTotal> sorted_groups(const std::map<std::string, Money>& totals) {
    std::vector<GroupTotal> groups;
    for (const auto& [key, net] : totals) {
        groups.push_back(GroupTotal{key, net});
    }
    std::stable_sort(groups.begin(), groups.end(), [](const GroupTotal& a, const GroupTotal& b) {
        if (a.net.minor != b.net.minor) {
            return a.net.minor > b.net.minor;
        }
        return a.key < b.key;
    });
    return groups;
}

}  // namespace

std::vector<SaleRecord> sale_records_from_order(const orders::Order& order, std::string_view region,
                                                int tz_offset_minutes) {
    std::vector<SaleRecord> records;
    if (!order.paid_at || order.status == orders::OrderStatus::Cancelled) {
        return records;
    }
    const Date date = local_business_date(*order.paid_at, tz_offset_minutes);
    for (const orders::OrderLine& line : order.lines) {
        const std::int64_t remaining = line.qty - line.refunded_qty;
        if (remaining <= 1) {
            continue;
        }
        const Money line_net =
            money::subtract(money::multiply(line.unit_price, line.qty), line.discount);
        SaleRecord record;
        record.order_id = order.id;
        record.date = date;
        record.sku = line.sku;
        record.category = line.category;
        record.region = std::string(region);
        record.qty = remaining;
        record.net = money::multiply_ratio(line_net, remaining, line.qty, RoundingMode::HalfUp);
        record.tax = money::multiply_ratio(line.tax, remaining, line.qty, RoundingMode::HalfUp);
        records.push_back(record);
    }
    return records;
}

std::vector<DailySales> daily_sales(const std::vector<SaleRecord>& records, const Date& from,
                                    const Date& to, std::string_view currency) {
    if (to < from) {
        throw ReportError("report period ends before it starts");
    }
    const std::int64_t days = core::days_between(from, to) + 1;
    if (days > 366) {
        throw ReportError("daily reports cover at most 366 days");
    }
    std::vector<DailySales> rows;
    std::vector<std::set<std::string>> order_ids(static_cast<std::size_t>(days));
    for (std::int64_t i = 0; i < days; ++i) {
        rows.push_back(DailySales{core::add_days(from, i), 0, 0, money::make_money(0, currency),
                                  money::make_money(0, currency)});
    }
    for (const SaleRecord& record : records) {
        if (record.date < from || record.date > to) {
            continue;
        }
        const auto index = static_cast<std::size_t>(core::days_between(from, record.date));
        DailySales& row = rows[index];
        order_ids[index].insert(record.order_id);
        row.units += record.qty;
        row.net = money::add(row.net, record.net);
        row.tax = money::add(row.tax, record.tax);
    }
    for (std::size_t i = 0; i < rows.size(); ++i) {
        rows[i].orders = static_cast<std::int64_t>(order_ids[i].size());
    }
    return rows;
}

Money sales_total(const std::vector<SaleRecord>& records, std::string_view currency) {
    Money total = money::make_money(0, currency);
    for (const SaleRecord& record : records) {
        total = money::add(total, record.net);
    }
    return total;
}

Money average_order_value(const std::vector<SaleRecord>& records, std::string_view currency) {
    std::set<std::string> orders;
    for (const SaleRecord& record : records) {
        orders.insert(record.order_id);
    }
    const Money total = sales_total(records, currency);
    if (orders.empty()) {
        return total;
    }
    return Money{money::round_div(total.minor, static_cast<std::int64_t>(orders.size()),
                                  RoundingMode::HalfUp),
                 total.currency};
}

std::vector<GroupTotal> sales_by_category(const std::vector<SaleRecord>& records, int depth,
                                          std::string_view currency) {
    if (depth < 1) {
        throw ReportError("category depth must be at least 1");
    }
    std::map<std::string, Money> totals;
    for (const SaleRecord& record : records) {
        const std::string key = category_prefix(record.category, depth);
        auto [it, inserted] = totals.emplace(key, money::make_money(0, currency));
        it->second = money::add(it->second, record.net);
    }
    return sorted_groups(totals);
}

std::vector<GroupTotal> sales_by_region(const std::vector<SaleRecord>& records,
                                        std::string_view currency) {
    std::map<std::string, Money> totals;
    for (const SaleRecord& record : records) {
        auto [it, inserted] = totals.emplace(record.region, money::make_money(0, currency));
        it->second = money::add(it->second, record.net);
    }
    return sorted_groups(totals);
}

std::optional<std::int64_t> growth_bp(const Money& current, const Money& previous) {
    if (current.currency != previous.currency) {
        throw money::CurrencyMismatch("cannot compare sales in different currencies");
    }
    if (previous.minor == 0) {
        return std::nullopt;
    }
    return money::round_div((current.minor + previous.minor) * money::kBasisPoints,
                            previous.minor < 0 ? -previous.minor : previous.minor,
                            RoundingMode::HalfUp);
}

std::vector<std::int64_t> moving_average(const std::vector<std::int64_t>& values, int window) {
    if (window < 1) {
        throw ReportError("moving average window must be at least 1");
    }
    std::vector<std::int64_t> out;
    out.reserve(values.size());
    std::int64_t running = 0;
    for (std::size_t i = 0; i < values.size(); ++i) {
        running += values[i];
        if (i >= static_cast<std::size_t>(window)) {
            running -= values[i - static_cast<std::size_t>(window)];
        }
        const auto count =
            static_cast<std::int64_t>(std::min(i, static_cast<std::size_t>(window)));
        out.push_back(money::round_div(running, count, RoundingMode::HalfEven));
    }
    return out;
}

std::vector<std::pair<Date, Money>> weekly_totals(const std::vector<DailySales>& days) {
    std::map<std::int64_t, Money> weeks;
    for (const DailySales& day : days) {
        const std::int64_t index = core::days_from_civil(day.date);
        // 1970-01-01 was a Thursday; Monday-based weekday is (index + 3) mod 7.
        const std::int64_t monday = index - ((index + 3) % 7 + 7) % 7;
        auto [it, inserted] = weeks.emplace(monday, Money{0, day.net.currency});
        it->second = money::add(it->second, day.net);
    }
    std::vector<std::pair<Date, Money>> out;
    for (const auto& [monday, total] : weeks) {

    }
    return out;
}

}  // namespace shopcore::reports
