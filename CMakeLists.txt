cmake_minimum_required(VERSION 3.28)

project(shopcore
    VERSION 1.0.0
    DESCRIPTION "ShopCore: back-office engine of the ShopCore online store"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SHOPCORE_COVERAGE "Instrument the library and tests for gcov line coverage" OFF)
option(SHOPCORE_BUILD_TESTS "Build the GoogleTest suites" ON)

set(SHOPCORE_MODULES money catalog inventory pricing tax cart orders shipping ledger reports)

# ---------------------------------------------------------------------------
# Library
# ---------------------------------------------------------------------------
file(GLOB_RECURSE SHOPCORE_SOURCES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/src/shopcore/*.cpp)
file(GLOB_RECURSE SHOPCORE_HEADERS CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/src/shopcore/*.hpp)

add_library(shopcore STATIC ${SHOPCORE_SOURCES} ${SHOPCORE_HEADERS})
target_include_directories(shopcore PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_compile_options(shopcore PRIVATE -Wall -Wextra)

if(SHOPCORE_COVERAGE)
    target_compile_options(shopcore PUBLIC --coverage -O0 -fno-inline)
    target_link_options(shopcore PUBLIC --coverage)
endif()

# ---------------------------------------------------------------------------
# Tests (one GoogleTest binary per module; see Makefile `test` target)
# ---------------------------------------------------------------------------
if(SHOPCORE_BUILD_TESTS)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_subdirectory(third_party/googletest EXCLUDE_FROM_ALL SYSTEM)

    # Seed data (data/*.csv) is embedded into the test-support library at configure
    # time so tests never touch the filesystem.
    set(SEED_FILES products tax_rules rate_table stock coupons fx_rates settlements)
    foreach(seed IN LISTS SEED_FILES)
        set(seed_path ${PROJECT_SOURCE_DIR}/data/${seed}.csv)
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${seed_path})
        file(READ ${seed_path} SEED_${seed})
    endforeach()
    configure_file(${PROJECT_SOURCE_DIR}/tests/support/seed_data.cpp.in
                   ${PROJECT_BINARY_DIR}/generated/seed_data.cpp @ONLY)

    add_library(shopcore_test_support STATIC
        ${PROJECT_BINARY_DIR}/generated/seed_data.cpp
        ${PROJECT_SOURCE_DIR}/tests/support/fixtures.cpp)
    target_include_directories(shopcore_test_support PUBLIC ${PROJECT_SOURCE_DIR}/tests)
    target_link_libraries(shopcore_test_support PUBLIC shopcore GTest::gtest)

    set(SHOPCORE_TEST_MODULES core ${SHOPCORE_MODULES})
    foreach(module IN LISTS SHOPCORE_TEST_MODULES)
        file(GLOB module_tests CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/tests/${module}/*_test.cpp)
        if(NOT module_tests)
            continue()
        endif()
        add_executable(shopcore_${module}_tests ${module_tests})
        target_link_libraries(shopcore_${module}_tests PRIVATE shopcore_test_support GTest::gtest_main)
        set_target_properties(shopcore_${module}_tests PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
    endforeach()
endif()
