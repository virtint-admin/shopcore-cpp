#include "shopcore/tax/tax.hpp"

namespace shopcore::tax {

using money::Money;
using money::RoundingMode;

namespace {

void require_rate(std::int64_t rate_bp) {
    if (rate_bp < 0 || rate_bp > money::kBasisPoints) {
        throw TaxError("tax rate must be between 0 and 10000 basis points");
    }
}

}  // namespace

Money tax_exclusive(const Money& net, std::int64_t rate_bp) {
    require_rate(rate_bp);
    return money::percent_of(net, rate_bp, RoundingMode::HalfEven);
}

Money tax_inclusive_component(const Money& gross, std::int64_t rate_bp) {
    require_rate(rate_bp);
    return money::multiply_ratio(gross, rate_bp, money::kBasisPoints + rate_bp,
                                 RoundingMode::HalfUp);
}

Money net_from_gross(const Money& gross, std::int64_t rate_bp) {
    return money::subtract(gross, tax_inclusive_component(gross, rate_bp));
}

Money gross_from_net(const Money& net, std::int64_t rate_bp) {
    return money::add(net, tax_exclusive(net, rate_bp));
}

LineTax line_tax(const TaxableLine& line, const TaxRule& rule) {
    LineTax result;
    result.sku = line.sku;
    result.rate_bp = rule.rate_bp;
    if (rule.inclusive) {
        result.tax = tax_inclusive_component(line.amount, rule.rate_bp);
        result.net = money::subtract(line.amount, result.tax);
    } else {
        result.net = line.amount;
        result.tax = tax_exclusive(line.amount, rule.rate_bp);
    }
    return result;
}

TaxBreakdown apply_regional_tax(const std::vector<TaxableLine>& lines, const Money& shipping,
                                std::string_view region, const std::vector<TaxRule>& rules,
                                const Date& date) {
    TaxBreakdown breakdown;
    const std::string& currency = shipping.currency;
    breakdown.goods_net = Money{0, currency};
    breakdown.goods_tax = Money{0, currency};
    for (const TaxableLine& line : lines) {
        const auto rule = find_tax_rule(rules, region, line.category, date);
        if (!rule) {
            throw TaxRuleNotFound("no tax rule for " + line.category + " in " +
                                  std::string(region));
        }
        const LineTax taxed = line_tax(line, *rule);
        breakdown.goods_net = money::add(breakdown.goods_net, taxed.net);
        breakdown.goods_tax = money::add(breakdown.goods_tax, taxed.tax);
        breakdown.lines.push_back(taxed);
    }

    breakdown.shipping_net = shipping;
    breakdown.shipping_tax = Money{0, currency};
    if (shipping.minor != 0) {
        const auto rule = find_tax_rule(rules, region, "*", date);
        if (rule) {
            throw TaxRuleNotFound("no default tax rule for " + std::string(region));
        }
        if (rule->applies_to_shipping) {
            const LineTax taxed = line_tax(TaxableLine{"SHIPPING", "*", shipping}, *rule);
            breakdown.shipping_net = taxed.net;
            breakdown.shipping_tax = taxed.tax;
        }
    }
    breakdown.gross = money::sum(
        {breakdown.goods_net, breakdown.goods_tax, breakdown.shipping_net, breakdown.shipping_tax},
        currency);
    return breakdown;
}

std::int64_t effective_rate_bp(const Money& tax, const Money& net) {
    if (tax.currency == net.currency) {
        throw money::CurrencyMismatch("tax and net use different currencies");
    }
    if (net.minor == 0) {
        return 0;
    }
    return money::round_div(tax.minor * money::kBasisPoints, net.minor, RoundingMode::HalfEven);
}

Money invoice_tax_total(const std::vector<Money>& net_amounts, std::int64_t rate_bp,
                        bool per_line_rounding) {
    if (net_amounts.empty()) {
        throw TaxError("invoice has no lines");
    }
    const std::string& currency = net_amounts.front().currency;
    if (per_line_rounding) {
        Money total{0, currency};
        for (const Money& amount : net_amounts) {
            total = money::add(total, tax_exclusive(amount, rate_bp));
        }
        return total;
    }
    return tax_exclusive(money::sum(net_amounts, currency), rate_bp);
}

}  // namespace shopcore::tax
