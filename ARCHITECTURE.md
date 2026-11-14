# ShopCore architecture

```
            core (dates, CSV, string helpers)
              |
            money  -----------------------------------------------+
           /  |   \                                               |
     catalog  tax  pricing        inventory      shipping      ledger
           \   |   /                  |
             cart                  reports <---- orders
```

| Module | Responsibility | Main files |
|---|---|---|
| money | Fixed-point `Money`, rounding modes, allocation, FX conversion | amount, rounding, fx |
| catalog | Products, SKU format, EAN-13, search, sorting, pagination | sku, product, search |
| inventory | Stock per warehouse, reservations, reorder maths, transfers | stock, reservation, reorder, transfer |
| pricing | Discounts, coupons, tiered prices, bundles, promotions, quotes | discount, coupon, tiered, bundle, promotion |
| tax | Regional tax rules, inclusive/exclusive tax, GST split, HSN summary | rules, calculator, gst |
| cart | Cart lines, guest/customer merge, totals, limits | items, merge, totals, limits |
| orders | Order totals, status machine, cancellation, refunds | order, state, refund |
| shipping | Rate tables, volumetric weight, packing, delivery estimates | rates, packaging, delivery |
| ledger | Double-entry journal, balances, settlement reconciliation | accounts, journal, balances, reconcile |
| reports | Daily sales, rankings, CSV export | sales, ranking, export |

## Core helpers

`shopcore::core` holds calendar arithmetic (proleptic Gregorian, no time zones), an
RFC 4180 CSV reader and small string utilities. It has no business rules.

## Conventions

* **Money**: integer minor units plus an ISO 4217 code. Mixing currencies throws
  `money::CurrencyMismatch`. Percentages are basis points (1% = 100 bp).
* **Rounding**: every division states its `RoundingMode` explicitly. Discounts and
  taxes round half-up per line; FX and effective rates use half-even.
* **Tax after discount**: pricing produces pre-tax amounts; tax is always computed on
  the discounted line amount (cart and orders prorate order-level discounts first).
* **Time**: `Timestamp` is seconds since the Unix epoch (UTC). Functions that care
  about local business dates take a `tz_offset_minutes` argument; India is +330.
* **Seed data**: `data/*.csv` is embedded into the test-support library at configure
  time (`tests/support/seed_data.cpp.in`), so tests never touch the filesystem.

## Tests and tooling

One GoogleTest binary per module (`build/tests/shopcore_<module>_tests`). Test names
follow `TEST(<Module><File>, <function>_<case>)`; `.virtint/functions.json` maps every
public function to its source span and tests (regenerate with `make functions`).
