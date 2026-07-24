#include <catch2/catch_test_macros.hpp>
#include "cord.hpp"

TEST_CASE("Line comments skipped", "[comments]") {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.add<int>("port");
    schema.add<std::string>("host");

    auto result = schema.parse("# comment\nport = 8080\n# another\nhost = \"localhost\"");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("port").as<int>() == 8080);
    CHECK(result.get("host").as<std::string>() == "localhost");
}

TEST_CASE("Comments disabled: comment line is malformed", "[comments]") {
    cord::Schema schema;
    schema.setAllowComments(false);
    schema.add<int>("port");

    auto result = schema.parse("# comment\nport = 8080");
    CHECK(result.hasErrors());
}

TEST_CASE("Inline comments stripped", "[comments]") {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.add<int>("port");
    schema.add<bool>("debug");

    auto result = schema.parse("port = 8080 # inline\ndebug = true # another");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("port").as<int>() == 8080);
    CHECK(result.get("debug").as<bool>() == true);
}

TEST_CASE("Inline comments disabled: hash in value causes error", "[comments]") {
    cord::Schema schema;
    schema.setAllowComments(false);
    schema.add<int>("port");

    auto result = schema.parse("port = 8080 # comment");
    CHECK(result.hasErrors());
}

TEST_CASE("Hash inside quoted string is not a comment", "[comments]") {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.add<std::string>("message");

    auto result = schema.parse("message = \"hello # not a comment\"");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("message").as<std::string>() == "hello # not a comment");
}

TEST_CASE("Inline comments after vector", "[comments]") {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.add<std::vector<int>>("ports");

    auto result = schema.parse("ports = [8080, 8081, 8082] # list of ports");
    REQUIRE_FALSE(result.hasErrors());
    auto ports = result.get("ports").as<std::vector<int>>();
    REQUIRE(ports.size() == 3);
    CHECK(ports[0] == 8080);
}
