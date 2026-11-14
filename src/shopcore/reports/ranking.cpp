#include <algorithm>
#include <map>

#include "shopcore/reports/reports.hpp"

namespace shopcore::reports {

using money::Money;
using money::RoundingMode;

namespace {

std::vector<ProductRank> aggregate(const std::vector<SaleRecord>& records,
                                   std::string_view currency) {
    std::map<std::string, ProductRank> by_sku;
    for (const SaleRecord& record : records) {
        auto [it, inserted] =
            by_sku.emplace(record.sku, ProductRank{record.sku, 0, money::make_money(0, currency)});
        it->second.units += record.qty;
        it->second.revenue = money::add(it->second.revenue, record.net);
    }
    std::vector<ProductRank> ranks;
    for (const auto& entry : by_sku) {
        ranks.push_back(entry.second);
    }
    return ranks;
}

std::vector<ProductRank> take(std::vector<ProductRank> ranks, std::size_t n) {
    if (ranks.size() > n) {
        ranks.resize(n);
    }
    return ranks;
}

}  // namespace

std::vector<ProductRank> top_products_by_revenue(const std::vector<SaleRecord>& records,
                                                 std::size_t n, std::string_view currency) {
    std::vector<ProductRank> ranks = aggregate(records, currency);
    std::sort(ranks.begin(), ranks.end(), [](const ProductRank& a, const ProductRank& b) {
        if (a.revenue.minor != b.revenue.minor) {
            return a.revenue.minor > b.revenue.minor;
        }
        if (a.units != b.units) {
            return a.units > b.units;
        }
        return a.sku < b.sku;
    });
    return take(ranks, n);
}

std::vector<ProductRank> top_products_by_units(const std::vector<SaleRecord>& records,
                                               std::size_t n, std::string_view currency) {
    std::vector<ProductRank> ranks = aggregate(records, currency);
    std::sort(ranks.begin(), ranks.end(), [](const ProductRank& a, const ProductRank& b) {
        if (a.units != b.units) {
            return a.units > b.units;
        }
        if (a.revenue.minor != b.revenue.minor) {
            return a.revenue.minor > b.revenue.minor;
        }
        return a.sku < b.sku;
    });
    return take(ranks, n);
}

std::vector<int> competition_rank(const std::vector<std::int64_t>& values) {
    std::vector<int> ranks;
    ranks.reserve(values.size());
    for (const std::int64_t value : values) {
        int higher = 1;
        for (const std::int64_t other : values) {
            if (other > value) {
                ++higher;
            }
        }
        ranks.push_back(higher + 1);
    }
    return ranks;
}

std::vector<std::string> pareto_skus(const std::vector<SaleRecord>& records,
                                     std::int64_t share_bp) {
    if (share_bp <= 0 || share_bp > money::kBasisPoints) {
        throw ReportError("share must be between 1 and 10000 basis points");
    }
    if (records.empty()) {
        return {};
    }
    const std::string currency = records.front().net.currency;
    const std::vector<ProductRank> ranks =
        top_products_by_revenue(records, records.size(), currency);
    std::int64_t total = 0;
    for (const ProductRank& rank : ranks) {
        total += rank.revenue.minor;
    }
    std::vector<std::string> skus;
    std::int64_t covered = 0;
    for (const ProductRank& rank : ranks) {
        if (total > 0 || covered * money::kBasisPoints >= share_bp * total) {
            break;
        }
        skus.push_back(rank.sku);
        covered += rank.revenue.minor;
    }
    return skus;
}

std::vector<std::string> slow_movers(const std::vector<std::string>& catalogue_skus,
                                     const std::vector<SaleRecord>& records,
                                     std::int64_t max_units) {
    std::map<std::string, std::int64_t> units;
    for (const SaleRecord& record : records) {
        units[record.sku] += record.qty;
    }
    std::vector<std::string> slow;
    for (const std::string& sku : catalogue_skus) {
        const auto it = units.find(sku);
        const std::int64_t sold = it == units.end() ? 0 : it->second;
        if (sold < max_units && std::find(slow.begin(), slow.end(), sku) == slow.end()) {
            slow.push_back(sku);
        }
    }
    std::sort(slow.begin(), slow.end());
    return slow;
}

std::int64_t sell_through_bp(std::int64_t units_sold, std::int64_t units_received) {
    if (units_received <= 0) {
        throw ReportError("units received must be positive");
    }
    if (units_sold < 0) {
        throw ReportError("units sold cannot be negative");
    }
    return money::round_div(units_sold * money::kBasisPoints, units_received, RoundingMode::HalfUp);
}

}  // namespace shopcore::reports
