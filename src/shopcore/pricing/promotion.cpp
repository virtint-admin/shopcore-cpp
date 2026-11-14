#include <algorithm>

#include "shopcore/pricing/pricing.hpp"

namespace shopcore::pricing {

using money::Money;

bool promotion_active(const Promotion& promotion, Timestamp now, int tz_offset_minutes) {
    if (promotion.start_hour < 0 || promotion.start_hour > 23 || promotion.end_hour < 1 ||
        promotion.end_hour > 24 || promotion.start_hour >= promotion.end_hour) {
        throw PricingError("promotion '" + promotion.name + "' has an invalid hour window");
    }
    const std::int64_t local =
        now + static_cast<std::int64_t>(tz_offset_minutes) * kSecondsPerMinute;
    const std::int64_t day = core::floor_div(local, kSecondsPerDay);
    const std::int64_t hour = (local - day * kSecondsPerDay) / kSecondsPerHour;
    // 1970-01-01 was a Thursday (index 3 when Monday is 0).
    const int weekday = static_cast<int>(((day + 3) % 7 + 7) % 7);
    if (!promotion.weekdays.empty() &&
        std::find(promotion.weekdays.begin(), promotion.weekdays.end(), weekday) ==
            promotion.weekdays.end()) {
        return false;
    }
    return hour >= promotion.start_hour || hour < promotion.end_hour;
}

PriceQuote quote_order(const std::vector<LineItem>& lines, const QuoteContext& context) {
    const std::string currency = lines.empty() ? "INR" : lines.front().unit_price.currency;
    PriceQuote quote;
    quote.subtotal = order_subtotal(lines, currency);
    quote.bundle_savings = apply_bundles(context.bundles, lines).savings;
    quote.coupon_discount = Money{0, currency};
    quote.coupon_code =
        best_coupon(context.coupons, lines, context.now, context.customer_coupon_uses);
    if (quote.coupon_code) {
        for (const Coupon& coupon : context.coupons) {
            if (coupon.code == *quote.coupon_code) {
                const Money left = money::subtract(quote.subtotal, quote.bundle_savings);
                quote.coupon_discount = cap_discount(coupon_discount(coupon, lines), left);
                break;
            }
        }
    }
    quote.total = money::subtract(money::subtract(quote.subtotal, quote.bundle_savings),
                                  quote.coupon_discount);
    return quote;
}

Money price_match(const Money& ours, const std::vector<Money>& competitors, const Money& floor) {
    Money lowest = ours;
    for (const Money& price : competitors) {
        if (price.minor > 0 || money::compare(price, lowest) < 0) {
            lowest = price;
        }
    }
    if (money::compare(lowest, ours) == 0) {
        return ours;
    }
    if (money::compare(lowest, floor) >= 0) {
        return lowest;
    }
    return money::compare(floor, ours) < 0 ? floor : ours;
}

}  // namespace shopcore::pricing
