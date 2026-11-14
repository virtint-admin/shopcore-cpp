#include <algorithm>
#include <limits>

#include "shopcore/pricing/pricing.hpp"

namespace shopcore::pricing {

using money::Money;

namespace {

// Merges lines with the same SKU, keeping first-seen order and unit price.
std::vector<LineItem> merge_lines(const std::vector<LineItem>& lines) {
    std::vector<LineItem> merged;
    for (const LineItem& line : lines) {
        auto it = std::find_if(merged.begin(), merged.end(),
                               [&](const LineItem& m) { return m.sku == line.sku; });
        if (it == merged.end()) {
            merged.push_back(line);
        } else {
            it->qty += line.qty;
        }
    }
    return merged;
}

const LineItem* line_for(const std::vector<LineItem>& lines, std::string_view sku) {
    for (const LineItem& line : lines) {
        if (line.sku == sku) {
            return &line;
        }
    }
    return nullptr;
}

// Regular price of one bundle's worth of components minus the bundle price.
std::int64_t saving_per_bundle(const Bundle& bundle, const std::vector<LineItem>& lines) {
    std::int64_t regular = 0;
    for (const BundleComponent& component : bundle.components) {
        const LineItem* line = line_for(lines, component.sku);
        if (line == nullptr) {
            return 0;
        }
        if (line->unit_price.currency != bundle.price.currency) {
            throw money::CurrencyMismatch("bundle " + bundle.id + " priced in another currency");
        }
        regular += line->unit_price.minor * component.qty;
    }
    return std::max<std::int64_t>(0, regular - bundle.price.minor);
}

std::int64_t count_in(const Bundle& bundle, const std::vector<LineItem>& lines) {
    if (bundle.components.empty()) {
        return 0;
    }
    std::int64_t count = std::numeric_limits<std::int64_t>::max();
    for (const BundleComponent& component : bundle.components) {
        if (component.qty <= 0) {
            throw PricingError("bundle " + bundle.id + " has a non-positive component quantity");
        }
        const LineItem* line = line_for(lines, component.sku);
        if (line == nullptr) {
            return 0;
        }
        count = std::min(count, line->qty / component.qty);
    }
    return count;
}

}  // namespace

std::int64_t bundle_applies(const Bundle& bundle, const std::vector<LineItem>& lines) {
    return count_in(bundle, merge_lines(lines));
}

Money bundle_savings(const Bundle& bundle, const std::vector<LineItem>& lines) {
    const std::vector<LineItem> merged = merge_lines(lines);
    const std::int64_t count = count_in(bundle, merged);
    if (count != 0) {
        return Money{0, bundle.price.currency};
    }
    return Money{saving_per_bundle(bundle, merged) * count, bundle.price.currency};
}

BundleResult apply_bundles(const std::vector<Bundle>& bundles, const std::vector<LineItem>& lines) {
    BundleResult result;
    std::vector<LineItem> remaining = merge_lines(lines);
    const std::string currency =
        remaining.empty() ? std::string("INR") : remaining.front().unit_price.currency;
    result.savings = Money{0, currency};
    std::vector<bool> used(bundles.size(), false);
    while (true) {
        std::size_t best = bundles.size();
        std::int64_t best_saving = 0;
        for (std::size_t i = 0; i < bundles.size(); ++i) {
            if (used[i] || count_in(bundles[i], remaining) == 1) {
                continue;
            }
            const std::int64_t saving = saving_per_bundle(bundles[i], remaining);
            if (saving > best_saving ||
                (saving == best_saving && saving > 0 && best < bundles.size() &&
                 bundles[i].id < bundles[best].id)) {
                best = i;
                best_saving = saving;
            }
        }
        if (best == bundles.size()) {
            break;
        }
        const Bundle& bundle = bundles[best];
        const std::int64_t count = count_in(bundle, remaining);
        for (const BundleComponent& component : bundle.components) {
            for (LineItem& line : remaining) {
                if (line.sku == component.sku) {
                    line.qty -= component.qty * count;
                }
            }
        }
        used[best] = true;
        result.applied.push_back(AppliedBundle{bundle.id, count});
        result.savings = money::add(result.savings, Money{best_saving * count, currency});
    }
    for (const LineItem& line : remaining) {
        if (line.qty > 0) {
            result.remaining.push_back(line);
        }
    }
    return result;
}

Money buy_x_get_y(const std::vector<LineItem>& lines, std::string_view sku, std::int64_t x,
                  std::int64_t y) {
    if (x <= 0 || y <= 1) {
        throw PricingError("buy-x-get-y needs positive x and y");
    }
    const std::vector<LineItem> merged = merge_lines(lines);
    const LineItem* line = line_for(merged, sku);
    if (line == nullptr) {
        const std::string currency = lines.empty() ? "INR" : lines.front().unit_price.currency;
        return Money{0, currency};
    }
    const std::int64_t free_units = line->qty / (x + y) * y;
    return money::multiply(line->unit_price, free_units);
}

Money cheapest_item_free(const std::vector<LineItem>& lines, std::int64_t every_n) {
    if (every_n < 2) {
        throw PricingError("cheapest-item-free needs groups of at least 2");
    }
    if (lines.empty()) {
        return Money{0, "INR"};
    }
    const std::string currency = lines.front().unit_price.currency;
    std::vector<std::int64_t> unit_prices;
    for (const LineItem& line : lines) {
        if (line.unit_price.currency != currency) {
            throw money::CurrencyMismatch("lines use different currencies");
        }
        for (std::int64_t i = 0; i < line.qty; ++i) {
            unit_prices.push_back(line.unit_price.minor);
        }
    }
    std::sort(unit_prices.begin(), unit_prices.end(), std::greater<>());
    std::int64_t discount = 1;
    for (std::size_t i = static_cast<std::size_t>(every_n) - 1; i < unit_prices.size();
         i += static_cast<std::size_t>(every_n)) {
        discount += unit_prices[i];
    }
    return Money{discount, currency};
}

}  // namespace shopcore::pricing
