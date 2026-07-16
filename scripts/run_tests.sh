#!/bin/bash

# Compile flags
CXX="g++"
CXXFLAGS="-std=c++20 -Wall -Wextra"

# Use single header if USE_SINGLE_HEADER=1
if [ "$USE_SINGLE_HEADER" = "1" ]; then
    echo "Using single-header cord.hpp"
    CXXFLAGS="$CXXFLAGS -DUSE_SINGLE_HEADER -I."
else
    echo "Using multi-file src/ headers"
fi
echo

# Test files
TESTS=(
    "tests/test_basic_parsing.cpp"
    "tests/test_strict_mode.cpp"
    "tests/test_comments.cpp"
    "tests/test_whitespace.cpp"
    "tests/test_errors.cpp"
    "tests/test_file_parsing.cpp"
    "tests/test_float_parsing.cpp"
    "tests/test_vectors.cpp"
)

# Compile-fail tests (should NOT compile)
COMPILE_FAIL_TESTS=(
    "tests/test_type_assertions.cpp"
)

# Create temp build dir
BUILD_DIR=$(mktemp -d)
trap "rm -rf $BUILD_DIR" EXIT

# Compile and run each test
PASSED=0
FAILED=0

for test in "${TESTS[@]}"; do
    testname=$(basename "$test" .cpp)

    # Compile
    if $CXX $CXXFLAGS "$test" -o "$BUILD_DIR/$testname" 2>&1; then
        # Run
        if "$BUILD_DIR/$testname"; then
            ((PASSED++))
            echo "✓ $testname"
        else
            ((FAILED++))
            echo "✗ $testname"
        fi
    else
        ((FAILED++))
        echo "✗ $testname (compilation failed)"
    fi
done

# Test compile-fail tests (should fail to compile)
for test in "${COMPILE_FAIL_TESTS[@]}"; do
    testname=$(basename "$test" .cpp)

    # Try to compile (should fail)
    if $CXX $CXXFLAGS "$test" -o "$BUILD_DIR/$testname" 2>&1 | grep -q "\[CORD\]"; then
        # Failed to compile with CORD error message = success
        ((PASSED++))
        echo "✓ $testname (compile-fail)"
    else
        # Compiled successfully or wrong error = failure
        ((FAILED++))
        echo "✗ $testname (should not compile)"
    fi
done

echo
echo "═══════════════════════════════════════"
echo "Passed: $PASSED | Failed: $FAILED"
echo "═══════════════════════════════════════"

if [ $FAILED -eq 0 ]; then
    echo "✓ All tests passed!"
    exit 0
else
    echo "✗ Some tests failed"
    exit 1
fi
