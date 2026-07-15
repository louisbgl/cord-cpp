#ifdef USE_SINGLE_HEADER
#include "cord.hpp"
#else
#include "../src/cord.hpp"
#endif

#include <cassert>
#include <iostream>

void test_comments_enabled() {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.add<int>("port");
    schema.add<std::string>("host");

    std::string config = R"(
# This is a comment
port = 8080
# Another comment
host = "localhost"
)";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    assert(result.get("port").as<int>() == 8080);
    assert(result.get("host").as<std::string>() == "localhost");

    std::cout << "✓ test_comments_enabled passed\n";
}

void test_comments_disabled() {
    cord::Schema schema;
    schema.setAllowComments(false);
    schema.add<int>("port");

    std::string config = R"(
# This is a comment
port = 8080
)";

    auto result = schema.parse(config);
    // Comment line treated as malformed (no '=')
    assert(result.hasErrors());

    std::cout << "✓ test_comments_disabled passed\n";
}

int main() {
    CORD_MAKE_LINTER_HAPPY();

    test_comments_enabled();
    test_comments_disabled();

    return 0;
}
