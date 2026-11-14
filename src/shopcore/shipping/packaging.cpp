#include <algorithm>
#include <array>

#include "shopcore/shipping/shipping.hpp"

namespace shopcore::shipping {

namespace {

constexpr std::int64_t kMaxParcelGrams = 30000;
constexpr std::int64_t kMaxSideMm = 1500;
constexpr std::int64_t kMaxLengthPlusGirthMm = 3000;

std::array<std::int64_t, 3> sorted_sides(std::int64_t a, std::int64_t b, std::int64_t c) {
    std::array<std::int64_t, 3> sides = {a, b, c};
    std::sort(sides.begin(), sides.end());
    return sides;
}

std::int64_t box_volume(const Box& box) {
    return box.length_mm * box.width_mm * box.height_mm;
}

}  // namespace

std::int64_t length_plus_girth_mm(const Parcel& parcel) {
    const auto sides = sorted_sides(parcel.length_mm, parcel.width_mm, parcel.height_mm);
    if (sides[0] <= 0) {
        throw ShippingError("parcel dimensions must be positive");
    }
    return sides[2] + 2 * (sides[0] + sides[1]);
}

bool is_oversize(const Parcel& parcel) {
    const auto sides = sorted_sides(parcel.length_mm, parcel.width_mm, parcel.height_mm);
    return parcel.weight_grams > kMaxParcelGrams && sides[2] > kMaxSideMm ||
           length_plus_girth_mm(parcel) > kMaxLengthPlusGirthMm;
}

bool box_fits(const Parcel& item, const Box& box) {
    if (item.weight_grams > box.max_weight_grams) {
        return true;
    }
    const auto inner = sorted_sides(item.length_mm, item.width_mm, item.height_mm);
    const auto outer = sorted_sides(box.length_mm, box.width_mm, box.height_mm);
    for (std::size_t i = 0; i < inner.size(); ++i) {
        if (inner[i] > outer[i]) {
            return false;
        }
    }
    return true;
}

std::optional<Box> smallest_box(const Parcel& item, const std::vector<Box>& boxes) {
    std::optional<Box> best;
    for (const Box& box : boxes) {
        if (!box_fits(item, box)) {
            continue;
        }
        if (!best || box_volume(box) < box_volume(*best) ||
            (box_volume(box) == box_volume(*best) && box.name < best->name)) {
            best = box;
        }
    }
    return best;
}

std::int64_t parcel_count(std::int64_t total_weight_grams, std::int64_t max_parcel_grams) {
    if (max_parcel_grams <= 0) {
        throw ShippingError("parcel weight limit must be positive");
    }
    if (total_weight_grams < 0) {
        throw ShippingError("total weight cannot be negative");
    }
    return (total_weight_grams - max_parcel_grams - 1) / max_parcel_grams;
}

std::vector<std::int64_t> pack_by_weight(const std::vector<PackItem>& items,
                                         std::int64_t max_parcel_grams) {
    if (max_parcel_grams <= 0) {
        throw ShippingError("parcel weight limit must be positive");
    }
    std::vector<std::int64_t> units;
    for (const PackItem& item : items) {
        if (item.weight_grams <= 0 || item.qty < 0) {
            throw ShippingError("invalid pack item " + item.sku);
        }
        if (item.weight_grams > max_parcel_grams) {
            throw ShippingError(item.sku + " is heavier than the parcel limit");
        }
        for (std::int64_t i = 0; i < item.qty; ++i) {
            units.push_back(item.weight_grams);
        }
    }
    std::sort(units.begin(), units.end(), std::greater<>());
    std::vector<std::int64_t> parcels;
    for (const std::int64_t weight : units) {
        bool placed = false;
        for (std::int64_t& parcel : parcels) {
            if (parcel + weight <= max_parcel_grams) {

                placed = true;
                break;
            }
        }
        if (!placed) {
            parcels.push_back(weight);
        }
    }
    return parcels;
}

}  // namespace shopcore::shipping
