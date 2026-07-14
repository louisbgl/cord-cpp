#include "../src/cord.hpp"
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

void test_basic_types() {
    cord::Schema schema;
    schema.add<int>("port");
    schema.add<double>("ratio");
    schema.add<bool>("debug");
    schema.add<std::string>("host");

    std::string config = R"(
port = 8080
ratio = 3.14
debug = true
host = "localhost"
)";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    assert(result.get("port").as<int>() == 8080);
    assert(result.get("ratio").as<double>() == 3.14);
    assert(result.get("debug").as<bool>() == true);
    assert(result.get("host").as<std::string>() == "localhost");

    std::cout << "✓ test_basic_types passed\n";
}

void test_defaults() {
    cord::Schema schema;
    schema.add<int>("port").default_(3000);
    schema.add<std::string>("host").default_("127.0.0.1");

    std::string config = "";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    assert(result.get("port").as<int>() == 3000);
    assert(result.get("host").as<std::string>() == "127.0.0.1");

    std::cout << "✓ test_defaults passed\n";
}

void test_required_fields() {
    cord::Schema schema;
    schema.add<int>("port").required();
    schema.add<std::string>("host").default_("localhost");

    std::string config = "host = \"example.com\"";

    auto result = schema.parse(config);
    assert(result.hasErrors());
    std::string errors = captureErrors(result);
    assert(errors.find("port") != std::string::npos);

    std::cout << "✓ test_required_fields passed\n";
}

void test_override_defaults() {
    cord::Schema schema;
    schema.add<int>("port").default_(3000);
    schema.add<bool>("debug").default_(false);

    std::string config = R"(
port = 8080
debug = true
)";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    assert(result.get("port").as<int>() == 8080);
    assert(result.get("debug").as<bool>() == true);

    std::cout << "✓ test_override_defaults passed\n";
}

int main() {
    CORD_MAKE_LINTER_HAPPY();
    
    test_basic_types();
    test_defaults();
    test_required_fields();
    test_override_defaults();

    std::cout << "\nAll basic parsing tests passed!\n";
    return 0;
}
