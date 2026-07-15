#ifdef USE_SINGLE_HEADER
#include "cord.hpp"
#else
#include "../src/cord.hpp"
#endif

#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>

// Helper to capture printErrors output
std::string captureErrors(const cord::Result& result) {
    std::stringstream ss;
    std::streambuf* old_cerr = std::cerr.rdbuf(ss.rdbuf());
    result.printErrors();
    std::cerr.rdbuf(old_cerr);
    return ss.str();
}

void test_parse_file_success() {
    // Create test config file
    std::ofstream out("test_config.txt");
    out << "port = 8080\n";
    out << "host = \"localhost\"\n";
    out << "debug = true\n";
    out.close();

    cord::Schema schema;
    schema.add<int>("port");
    schema.add<std::string>("host");
    schema.add<bool>("debug");

    auto result = schema.parseFile("test_config.txt");
    assert(!result.hasErrors());
    assert(result.get("port").as<int>() == 8080);
    assert(result.get("host").as<std::string>() == "localhost");
    assert(result.get("debug").as<bool>() == true);

    // Cleanup
    std::remove("test_config.txt");

    std::cout << "✓ test_parse_file_success passed\n";
}

void test_parse_file_not_found() {
    cord::Schema schema;
    schema.add<int>("port");

    auto result = schema.parseFile("nonexistent_file.txt");
    assert(result.hasErrors());
    std::string errors = captureErrors(result);
    assert(errors.find("Failed to open file") != std::string::npos);

    std::cout << "✓ test_parse_file_not_found passed\n";
}

int main() {
    test_parse_file_success();
    test_parse_file_not_found();

    return 0;
}
