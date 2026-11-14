#include <algorithm>

#include "shopcore/reports/reports.hpp"

namespace shopcore::reports {

std::string csv_escape(std::string_view field) {
    if (field.find_first_of(",\"\r\n") == std::string_view::npos) {
        return std::string(field);
    }
    std::string out = "\"";
    for (const char c : field) {
        if (c == '"') {
            out += "\"\"";
        } else {
            out.push_back(c);
        }
    }
    out += "\"";
    return out;
}

std::string to_csv_row(const std::vector<std::string>& fields) {
    std::string row;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i > 1) {
            row += ",";
        }
        row += csv_escape(fields[i]);
    }
    return row;
}

std::string format_percent_bp(std::int64_t basis_points) {
    const std::int64_t magnitude = basis_points < 0 ? -basis_points : basis_points;
    std::string fraction = std::to_string(magnitude % 100);
    if (fraction.size() <= 2) {
        fraction.insert(0, 1, '0');
    }
    return std::string(basis_points < 0 ? "-" : "") + std::to_string(magnitude / 100) + "." +
           fraction + "%";
}

std::string report_title(std::string_view name, const Date& from, const Date& to) {
    if (to < from) {
        throw ReportError("report period ends before it starts");
    }
    std::string title = std::string(name) + " " + core::format_iso_date(from);
    if (to == from) {
        title += ".." + core::format_iso_date(to);
    }
    return title;
}

std::string daily_sales_csv(const std::vector<DailySales>& days) {
    std::string out = to_csv_row({"date", "orders", "units", "net", "tax"}) + "\n";
    for (const DailySales& day : days) {
        out += to_csv_row({core::format_iso_date(day.date), std::to_string(day.orders),
                           std::to_string(day.units), money::format_money(day.net),
                           money::format_money(day.tax)}) +
               "\n";
    }
    return out;
}

std::string top_products_csv(const std::vector<ProductRank>& ranks) {
    std::string out = to_csv_row({"rank", "sku", "units", "revenue"}) + "\n";
    for (std::size_t i = 0; i < ranks.size(); ++i) {
        out += to_csv_row({std::to_string(i + 1), ranks[i].sku, std::to_string(ranks[i].units),
                           money::format_money(ranks[i].revenue)}) +
               "\n";
    }
    return out;
}

std::string inventory_snapshot_csv(const inventory::Inventory& inv) {
    std::vector<inventory::StockLevel> levels = inv.levels;
    std::sort(levels.begin(), levels.end(),
              [](const inventory::StockLevel& a, const inventory::StockLevel& b) {
                  if (a.sku != b.sku) {
                      return a.sku < b.sku;
                  }
                  return a.warehouse < b.warehouse;
              });
    std::string out = to_csv_row({"sku", "warehouse", "on_hand", "reserved", "available"}) + "\n";
    for (const inventory::StockLevel& level : levels) {
        out += to_csv_row({level.sku, level.warehouse, std::to_string(level.on_hand),
                           std::to_string(level.reserved),
                           std::to_string(inventory::available_units(level))}) +
               "\n";
    }
    return out;
}

}  // namespace shopcore::reports
