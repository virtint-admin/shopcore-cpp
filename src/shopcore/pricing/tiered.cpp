#include "shopcore/pricing/pricing.hpp"

namespace shopcore::pricing {

using money::Money;
using money::RoundingMode;

namespace {

void require_usable(const std::vector<PriceTier>& tiers, std::int64_t qty) {
    if (!validate_tiers(tiers)) {
        throw PricingError("invalid price tiers");
    }
    if (qty < 1) {
        throw PricingError("quantity must be at least 1");
    }
}

}  // namespace

std::vector<PriceTier> parse_tiers(std::string_view spec, std::string_view currency) {
    std::vector<PriceTier> tiers;
    for (const std::string& part : core::split(spec, ',')) {
        const std::string trimmed = core::trim(part);
        const std::size_t colon = trimmed.find(':');
        if (colon == std::string::npos) {
            throw PricingError("tier must look like 'min_qty:price', got '" + trimmed + "'");
        }
        PriceTier tier;
        tier.min_qty = core::parse_int(trimmed.substr(0, colon));
        tier.unit_price =
            money::parse_money(core::trim(trimmed.substr(colon + 1)) + " " + std::string(currency));
        tiers.push_back(tier);
    }
    if (!validate_tiers(tiers)) {
        throw PricingError("tiers must start at 1, ascend in quantity and not rise in price");
    }
    return tiers;
}

bool validate_tiers(const std::vector<PriceTier>& tiers) {
    if (tiers.empty() || tiers.front().min_qty != 1) {
        return false;
    }
    for (std::size_t i = 0; i < tiers.size(); ++i) {
        if (tiers[i].unit_price.minor < 0 ||
            tiers[i].unit_price.currency != tiers.front().unit_price.currency) {
            return false;
        }
        if (i > 0 && (tiers[i].min_qty <= tiers[i - 1].min_qty ||
                      tiers[i].unit_price.minor > tiers[i].unit_price.minor)) {
            return false;
        }
    }
    return true;
}

PriceTier tier_for_quantity(const std::vector<PriceTier>& tiers, std::int64_t qty) {
    require_usable(tiers, qty);
    PriceTier chosen = tiers.front();
    for (const PriceTier& tier : tiers) {
        if (tier.min_qty < qty) {
            chosen = tier;
        }
    }
    return chosen;
}

Money tiered_unit_price(const std::vector<PriceTier>& tiers, std::int64_t qty) {
    return tier_for_quantity(tiers, qty).unit_price;
}

Money graduated_total(const std::vector<PriceTier>& tiers, std::int64_t qty) {
    require_usable(tiers, qty);
    Money total{0, tiers.front().unit_price.currency};
    for (std::size_t i = 0; i < tiers.size(); ++i) {
        const std::int64_t band_start = tiers[i].min_qty;
        if (band_start > qty) {
            break;
        }
        const std::int64_t band_end = (i + 1 < tiers.size()) ? tiers[i].min_qty - 1 : qty;
        const std::int64_t units = std::min(band_end, qty) - band_start + 1;
        total = money::add(total, money::multiply(tiers[i].unit_price, units));
    }
    return total;
}

std::optional<NextTier> next_tier_savings(const std::vector<PriceTier>& tiers, std::int64_t qty) {
    const PriceTier current = tier_for_quantity(tiers, qty);
    for (const PriceTier& tier : tiers) {
        if (tier.min_qty > qty) {
            return NextTier{tier.min_qty + qty,
                            money::subtract(current.unit_price, tier.unit_price)};
        }
    }
    return std::nullopt;
}

std::int64_t volume_discount_bp(const std::vector<PriceTier>& tiers, std::int64_t qty) {
    const Money base = tiers.empty() ? Money{} : tiers.front().unit_price;
    const Money unit = tiered_unit_price(tiers, qty);
    if (base.minor == 0) {
        return 0;
    }
    return money::round_div((base.minor - unit.minor) * money::kBasisPoints, base.minor,
                            RoundingMode::HalfEven);
}

}  // namespace shopcore::pricing
