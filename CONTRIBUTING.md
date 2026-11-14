# Contributing to ShopCore

1. Fork the repository and branch from `main`.
2. Reproduce the bug with the failing test from the ticket:
   `make test TESTS="<Suite.name>"`.
3. Fix the code under `src/`. Do **not** edit `tests/`, `Makefile`, `CMakeLists.txt`,
   `third_party/`, `.github/` or `CODEOWNERS`: pull requests that touch
   them are rejected automatically (see `.virtint/protected.txt`).
4. Run your issue's tests again and the linter before opening the PR:
   `make test TESTS="<Suite.name>" && make lint`. `main` carries every open issue, so in
   a full `make test` run the tests named in other open issues fail. That is expected;
   your change must not break any other test.
5. Keep the change small and focused; explain the root cause in the PR description.

## Code style

* C++20, formatted with the repository `.clang-format` (`clang-format -i <file>`).
* Money is always `money::Money` (integer minor units + currency). Never use floating
  point for amounts; use the rounding helpers in `money/rounding.cpp`.
* Time is injected by the caller (`Timestamp`, `Date`); ShopCore never reads the clock.
* Errors are reported with exceptions derived from `shopcore::DomainError`.
