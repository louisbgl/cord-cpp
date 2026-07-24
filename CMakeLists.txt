cmake_minimum_required(VERSION 3.15)
project(cord CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(cord INTERFACE)
target_include_directories(cord INTERFACE ${CMAKE_SOURCE_DIR}/src)

option(CORD_BUILD_TESTS    "Build tests"    ON)
option(CORD_BUILD_EXAMPLES "Build examples" ON)

option(CORD_COVERAGE "Enable coverage reporting" OFF)

if(CORD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)

    if(CORD_COVERAGE)
        set(COVERAGE_INFO ${CMAKE_BINARY_DIR}/coverage.info)
        set(COVERAGE_HTML ${CMAKE_BINARY_DIR}/coverage_html)

        add_custom_target(coverage
            COMMAND ctest --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
            COMMAND lcov --capture --directory ${CMAKE_BINARY_DIR} --output-file ${COVERAGE_INFO} --quiet --ignore-errors gcov,gcov
            COMMAND lcov --remove ${COVERAGE_INFO} "*/catch2/*" "*/c++/*"
                --output-file ${COVERAGE_INFO} --quiet --ignore-errors unused
            COMMAND lcov --summary ${COVERAGE_INFO}
            DEPENDS cord_tests
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running tests and printing coverage summary"
        )

        add_custom_target(coverage-html
            COMMAND ctest --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
            COMMAND lcov --capture --directory ${CMAKE_BINARY_DIR} --output-file ${COVERAGE_INFO} --quiet --ignore-errors gcov,gcov
            COMMAND lcov --remove ${COVERAGE_INFO} "*/catch2/*" "*/c++/*"
                --output-file ${COVERAGE_INFO} --quiet --ignore-errors unused
            COMMAND genhtml ${COVERAGE_INFO} --output-directory ${COVERAGE_HTML} --quiet
            COMMAND ${CMAKE_COMMAND} -E echo "Report: ${COVERAGE_HTML}/index.html"
            DEPENDS cord_tests
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Generating HTML coverage report"
        )
    endif()
endif()

if(CORD_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
