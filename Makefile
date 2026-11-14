# ShopCore C++ build entry points (used by .github/workflows/virtint-review.yml).
#
#   make build                      configure + compile library and test binaries
#   make test [TESTS="A.b;C.d"]     run tests, JUnit XML in reports/junit/*.xml
#   make lint                       clang-tidy + clang-format on src/, reports/lint.json
#   make coverage                   instrumented run, Cobertura XML in reports/coverage.xml
#   make functions                  regenerate .virtint/functions.json
#
# GNU make itself always exits 2 when a recipe fails, so `make test` and `make coverage`
# also write the test runner's own exit code to reports/exit-code:
#   0 = all tests passed, 1 = at least one test failed, >= 2 = build/infrastructure error.
# Everything works offline: GoogleTest is vendored in third_party/ and the scripts are
# stdlib-only Python 3.

BUILD_DIR    ?= build
COV_DIR      ?= build-coverage
REPORTS      ?= reports
TESTS        ?=
JOBS         ?= $(shell nproc 2>/dev/null || echo 2)
CMAKE        ?= cmake
PYTHON       ?= python3
CMAKE_FLAGS  ?=

export PYTHONDONTWRITEBYTECODE := 1

.PHONY: all build test lint coverage functions clean

all: build

$(BUILD_DIR)/CMakeCache.txt:
	$(CMAKE) -S . -B $(BUILD_DIR) -G "Unix Makefiles" $(CMAKE_FLAGS)

build: $(BUILD_DIR)/CMakeCache.txt
	$(CMAKE) --build $(BUILD_DIR) --parallel $(JOBS)

test:
	@mkdir -p $(REPORTS); rm -f $(REPORTS)/exit-code; \
	if ! $(MAKE) --no-print-directory build; then \
		echo 2 > $(REPORTS)/exit-code; echo "make test: build failed" >&2; exit 2; \
	fi; \
	$(PYTHON) scripts/run_tests.py --build-dir $(BUILD_DIR) --reports $(REPORTS) --tests "$(TESTS)"; \
	rc=$$?; echo $$rc > $(REPORTS)/exit-code; exit $$rc

lint:
	@mkdir -p $(REPORTS)
	@test -f $(BUILD_DIR)/compile_commands.json || $(CMAKE) -S . -B $(BUILD_DIR) -G "Unix Makefiles" $(CMAKE_FLAGS) >/dev/null
	$(PYTHON) scripts/lint.py --build-dir $(BUILD_DIR) --output $(REPORTS)/lint.json --jobs $(JOBS)

coverage:
	@mkdir -p $(REPORTS); rm -f $(REPORTS)/exit-code $(REPORTS)/coverage.xml; \
	if ! { $(CMAKE) -S . -B $(COV_DIR) -G "Unix Makefiles" -DSHOPCORE_COVERAGE=ON $(CMAKE_FLAGS) >/dev/null && \
	       $(CMAKE) --build $(COV_DIR) --parallel $(JOBS); }; then \
		echo 2 > $(REPORTS)/exit-code; echo "make coverage: build failed" >&2; exit 2; \
	fi; \
	find $(COV_DIR) -name '*.gcda' -delete; \
	$(PYTHON) scripts/run_tests.py --build-dir $(COV_DIR) --reports $(REPORTS) --tests "$(TESTS)"; \
	rc=$$?; \
	if [ $$rc -lt 2 ]; then \
		$(PYTHON) scripts/coverage_xml.py --build-dir $(COV_DIR) --source-root . --output $(REPORTS)/coverage.xml || rc=2; \
	fi; \
	echo $$rc > $(REPORTS)/exit-code; exit $$rc

functions:
	$(PYTHON) scripts/gen_functions_json.py --check

clean:
	rm -rf $(BUILD_DIR) $(COV_DIR) $(REPORTS)
