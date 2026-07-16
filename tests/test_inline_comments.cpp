#ifdef USE_SINGLE_HEADER
#include "cord.hpp"
#else
#include "../src/cord.hpp"
#endif

#include <cassert>
#include <iostream>

void test_inline_comments_ignored() {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.add<int>("port");
    schema.add<std::string>("host");
    schema.add<bool>("debug");

    std::string config = R"(
port = 8080 # comment after value
host = "localhost" # another comment
debug = true # yet another comment
)";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    assert(result.get("port").as<int>() == 8080);
    assert(result.get("host").as<std::string>() == "localhost");
    assert(result.get("debug").as<bool>() == true);

    std::cout << "✓ test_inline_comments_ignored passed\n";
}

void test_inline_comments_disabled() {
    cord::Schema schema;
    schema.setAllowComments(false);
    schema.add<int>("port");

    std::string config = "port = 8080 # comment";

    auto result = schema.parse(config);
    assert(result.hasErrors());

    std::cout << "✓ test_inline_comments_disabled passed\n";
}

void test_inline_comments_in_strings() {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.add<std::string>("message");

    std::string config = "message = \"hello # not a comment\"";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    assert(result.get("message").as<std::string>() == "hello # not a comment");

    std::cout << "✓ test_inline_comments_in_strings passed\n";
}

void test_inline_comments_with_vectors() {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.add<std::vector<int>>("ports");

    std::string config = "ports = [8080, 8081, 8082] # list of ports";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    auto ports = result.get("ports").as<std::vector<int>>();
    assert(ports.size() == 3);
    assert(ports[0] == 8080);
    assert(ports[1] == 8081);
    assert(ports[2] == 8082);

    std::cout << "✓ test_inline_comments_with_vectors passed\n";
}

int main() {
    test_inline_comments_ignored();
    test_inline_comments_disabled();
    test_inline_comments_in_strings();
    test_inline_comments_with_vectors();

    return 0;
}
