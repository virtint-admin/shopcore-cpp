# ShopCore (C++)

> **Learners:** start with [LEARNERS.md](LEARNERS.md), how to pick an issue, fix it and get it checked.

ShopCore is the back-office engine of the ShopCore online store: money handling, the
product catalogue, inventory, pricing and coupons, GST/VAT, carts, orders, shipping,
the finance ledger and sales reports. It is a plain C++20 static library with a
GoogleTest suite; there is no server, database or network access.

## Quick start

```sh
make build        # configure (CMake >= 3.28, GCC 13) and compile library + tests
make test         # run every test; JUnit XML in reports/junit/
make test TESTS="TaxCalculator.apply_regional_tax_excludes_shipping"
make lint         # clang-tidy + clang-format on src/; reports/lint.json
make coverage     # line coverage; Cobertura XML in reports/coverage.xml
```

`main` carries every open issue: in a full `make test` run the tests named in open
issues fail until someone fixes them, and everything else passes.

`make test` writes the runner's exit code to `reports/exit-code`
(0 = all passed, 1 = a test failed, 2 = build or infrastructure error).

Everything builds offline: GoogleTest is vendored in `third_party/googletest`.

## Layout

```
src/shopcore/<module>/<module>.hpp   public API of a module
src/shopcore/<module>/<file>.cpp     implementation
tests/<module>/<file>_test.cpp       tests for that file, TEST(Suite, <function>_<case>)
data/*.csv                           seed data used by tests and reports
scripts/                             test runner, lint, coverage and index helpers
```

Modules: `money`, `catalog`, `inventory`, `pricing`, `tax`, `cart`, `orders`,
`shipping`, `ledger`, `reports` (plus `core`, shared date/CSV helpers).
See [ARCHITECTURE.md](ARCHITECTURE.md) for how they fit together and
[CONTRIBUTING.md](CONTRIBUTING.md) for the workflow.
