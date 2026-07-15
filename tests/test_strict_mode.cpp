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

void test_strict_mode_rejects_unknown() {
    cord::Schema schema;
    schema.setStrict(true);
    schema.add<int>("port");

    std::string config = R"(
port = 8080
unknown_key = 123
)";

    auto result = schema.parse(config);
    assert(result.hasErrors());
    std::string errors = captureErrors(result);
    assert(errors.find("unknown_key") != std::string::npos);

    std::cout << "✓ test_strict_mode_rejects_unknown passed\n";
}

void test_lenient_mode_ignores_unknown() {
    cord::Schema schema;
    schema.setStrict(false);
    schema.add<int>("port");

    std::string config = R"(
port = 8080
unknown_key = 123
another_unknown = "value"
)";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    assert(result.get("port").as<int>() == 8080);

    std::cout << "✓ test_lenient_mode_ignores_unknown passed\n";
}

int main() {
    test_strict_mode_rejects_unknown();
    test_lenient_mode_ignores_unknown();

    return 0;
}
