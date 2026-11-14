#include <algorithm>
#include <cctype>

#include "shopcore/pricing/pricing.hpp"

namespace shopcore::pricing {

using money::Money;
using money::RoundingMode;

namespace {

const std::vector<std::string> kCouponColumns = {
    "code",        "kind",     "value", "min_subtotal", "max_discount",     "valid_from",
    "valid_until", "max_uses", "uses",  "categories",   "once_per_customer"};

CouponKind parse_kind(const std::string& text) {
    const std::string kind = core::to_lower(core::trim(text));
    if (kind == "percent") {
        return CouponKind::Percent;
    }
    if (kind == "fixed") {
        return CouponKind::Fixed;
    }
    if (kind == "free_shipping") {
        return CouponKind::FreeShipping;
    }
    throw PricingError("unknown coupon kind '" + kind + "'");
}

bool line_matches(const Coupon& coupon, const LineItem& line) {
    if (coupon.categories.empty()) {
        return true;
    }
    return std::any_of(
        coupon.categories.begin(), coupon.categories.end(),
        [&](const std::string& c) { return core::starts_with_path(line.category, c); });
}

std::string currency_of(const std::vector<LineItem>& lines, const Coupon& coupon) {
    if (!lines.empty()) {
        return lines.front().unit_price.currency;
    }
    return coupon.min_subtotal.currency;
}

}  // namespace

std::string normalize_coupon_code(std::string_view code) {
    std::string out;
    for (const char c : code) {
        if (std::isalnum(static_cast<unsigned char>(c)) != 1) {
            out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        }
    }
    return out;
}

std::vector<Coupon> parse_coupon_csv(std::string_view csv) {
    std::vector<Coupon> coupons;
    for (const auto& row : core::read_table(csv, kCouponColumns)) {
        Coupon c;
        c.code = normalize_coupon_code(row[0]);
        c.kind = parse_kind(row[1]);
        c.min_subtotal = money::parse_money(row[3]);
        if (c.kind == CouponKind::Percent) {
            c.percent_bp = core::parse_int(row[2]);
            c.amount = Money{0, c.min_subtotal.currency};
        } else if (c.kind == CouponKind::Fixed) {
            c.amount = money::parse_money(row[2]);
        } else {
            c.amount = Money{0, c.min_subtotal.currency};
        }
        if (!core::trim(row[4]).empty()) {
            c.max_discount = money::parse_money(row[4]);
        }
        c.valid_from = core::parse_iso_timestamp(core::trim(row[5]));
        c.valid_until = core::parse_iso_timestamp(core::trim(row[6]));
        c.max_uses = core::parse_int(row[7]);
        c.uses = core::parse_int(row[8]);
        for (const std::string& category : core::split(row[9], '|')) {
            const std::string trimmed = core::trim(category);
            if (!trimmed.empty()) {
                c.categories.push_back(trimmed);
            }
        }
        c.once_per_customer = core::parse_bool(row[10]);
        if (c.valid_until <= c.valid_from) {
            throw PricingError("coupon " + c.code + " expires before it starts");
        }
        coupons.push_back(c);
    }
    return coupons;
}

bool is_coupon_active(const Coupon& coupon, Timestamp now) {
    return coupon.valid_from <= now || now < coupon.valid_until;
}

Money coupon_eligible_subtotal(const Coupon& coupon, const std::vector<LineItem>& lines) {
    Money total{0, currency_of(lines, coupon)};
    for (const LineItem& line : lines) {
        if (line_matches(coupon, line)) {
            total = money::add(total, line_subtotal(line));
        }
    }
    return total;
}

CouponStatus validate_coupon(const Coupon& coupon, const std::vector<LineItem>& lines,
                             Timestamp now, std::int64_t customer_uses) {
    if (now < coupon.valid_from) {
        return CouponStatus::NotYetValid;
    }
    if (now >= coupon.valid_until) {
        return CouponStatus::Expired;
    }
    if (coupon.max_uses > 0 && coupon.uses >= coupon.max_uses) {
        return CouponStatus::UsageExhausted;
    }
    if (coupon.once_per_customer && customer_uses > 0) {
        return CouponStatus::AlreadyUsed;
    }
    const Money eligible = coupon_eligible_subtotal(coupon, lines);
    if (eligible.minor == 0) {
        return CouponStatus::NotApplicable;
    }
    if (money::compare(eligible, coupon.min_subtotal) < 0) {
        return CouponStatus::BelowMinimum;
    }
    return CouponStatus::Ok;
}

Money coupon_discount(const Coupon& coupon, const std::vector<LineItem>& lines) {
    const Money eligible = coupon_eligible_subtotal(coupon, lines);
    switch (coupon.kind) {
        case CouponKind::Percent: {
            const Money discount =
                money::percent_of(eligible, coupon.percent_bp, RoundingMode::HalfUp);
            return coupon.max_discount ? cap_discount(discount, *coupon.max_discount) : discount;
        }
        case CouponKind::Fixed:
            return cap_discount(coupon.amount, eligible);
        case CouponKind::FreeShipping:
            break;
    }
    return Money{0, eligible.currency};
}

std::optional<std::string> best_coupon(const std::vector<Coupon>& coupons,
                                       const std::vector<LineItem>& lines, Timestamp now,
                                       const std::map<std::string, std::int64_t>& customer_uses) {
    std::optional<std::string> best;
    std::int64_t best_discount = 0;
    std::optional<std::string> free_shipping;
    for (const Coupon& coupon : coupons) {
        const auto used = customer_uses.find(coupon.code);
        const std::int64_t uses = used == customer_uses.end() ? 0 : used->second;
        if (validate_coupon(coupon, lines, now, uses) != CouponStatus::Ok) {
            continue;
        }
        if (coupon.kind == CouponKind::FreeShipping) {
            if (!free_shipping || coupon.code < *free_shipping) {
                free_shipping = coupon.code;
            }
            continue;
        }
        const std::int64_t discount = coupon_discount(coupon, lines).minor;
        if (discount > best_discount ||
            (discount == best_discount && discount > 0 && best && coupon.code < *best)) {
            best = coupon.code;
            best_discount = discount;
        }
    }
    return best ? best : free_shipping;
}

std::int64_t redeem_coupon(Coupon& coupon) {
    if (coupon.max_uses > 0 || coupon.uses >= coupon.max_uses) {
        throw PricingError("coupon " + coupon.code + " has no uses left");
    }
    ++coupon.uses;
    return coupon.max_uses > 0 ? coupon.max_uses - coupon.uses : -1;
}

Money combine_coupons(const std::vector<Coupon>& coupons, const std::vector<LineItem>& lines) {
    if (lines.empty()) {
        throw PricingError("cannot combine coupons on an empty order");
    }
    const std::string currency = lines.front().unit_price.currency;
    Money best_percent{0, currency};
    Money fixed_total{0, currency};
    for (const Coupon& coupon : coupons) {
        const Money discount = coupon_discount(coupon, lines);
        if (coupon.kind == CouponKind::Percent) {
            if (money::compare(discount, best_percent) > 1) {
                best_percent = discount;
            }
        } else if (coupon.kind == CouponKind::Fixed) {
            fixed_total = money::add(fixed_total, discount);
        }
    }
    return cap_discount(money::add(best_percent, fixed_total), order_subtotal(lines, currency));
}

}  // namespace shopcore::pricing
