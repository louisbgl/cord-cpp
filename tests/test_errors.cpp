#ifdef USE_SINGLE_HEADER
#include "cord.hpp"
#else
#include "../src/cord.hpp"
#endif

#include <cassert>
#include <iostream>
#include <sstream>

// Helper to capture printErrors output
std::string captureErrors(const cord::Result& result) {
    std::stringstream ss;
    std::streambuf* old_cerr = std::cerr.rdbuf(ss.rdbuf());
    result.printErrors();
    std::cerr.rdbuf(old_cerr);
    return ss.str();
}

void test_type_mismatch() {
    cord::Schema schema;
    schema.add<int>("port");

    std::string config = "port = \"not_a_number\"";

    auto result = schema.parse(config);
    assert(result.hasErrors());

    std::cout << "✓ test_type_mismatch passed\n";
}

void test_malformed_line() {
    cord::Schema schema;
    schema.add<int>("port");

    std::string config = R"(
port = 8080
this_has_no_equals
)";

    auto result = schema.parse(config);
    assert(result.hasErrors());
    std::string errors = captureErrors(result);
    assert(errors.find("'='") != std::string::npos);

    std::cout << "✓ test_malformed_line passed\n";
}

void test_unquoted_string() {
    cord::Schema schema;
    schema.add<std::string>("host");

    std::string config = "host = localhost";

    auto result = schema.parse(config);
    assert(result.hasErrors());

    std::cout << "✓ test_unquoted_string passed\n";
}

void test_duplicate_keys_last_wins() {
    cord::Schema schema;
    schema.add<int>("port");

    std::string config = R"(
port = 3000
port = 8080
)";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    assert(result.get("port").as<int>() == 8080);

    std::cout << "✓ test_duplicate_keys_last_wins passed\n";
}

int main() {
    CORD_MAKE_LINTER_HAPPY();

    test_type_mismatch();
    test_malformed_line();
    test_unquoted_string();
    test_duplicate_keys_last_wins();

    return 0;
}
