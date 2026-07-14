#include "../src/cord.hpp"
#include <cassert>
#include <iostream>

void test_whitespace_trimming() {
    cord::Schema schema;
    schema.add<int>("port");
    schema.add<std::string>("host");

    std::string config = R"(
   port   =   8080
host="localhost"
)";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    assert(result.get("port").as<int>() == 8080);
    assert(result.get("host").as<std::string>() == "localhost");

    std::cout << "✓ test_whitespace_trimming passed\n";
}

void test_empty_lines() {
    cord::Schema schema;
    schema.add<int>("port");
    schema.add<bool>("debug");

    std::string config = R"(

port = 8080


debug = true

)";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    assert(result.get("port").as<int>() == 8080);
    assert(result.get("debug").as<bool>() == true);

    std::cout << "✓ test_empty_lines passed\n";
}

int main() {
    CORD_MAKE_LINTER_HAPPY();
    
    test_whitespace_trimming();
    test_empty_lines();

    std::cout << "\nAll whitespace tests passed!\n";
    return 0;
}
