#include <catch2/catch_test_macros.hpp>
#include "cord.hpp"

TEST_CASE("Custom char delimiter", "[markers]") {
    cord::Schema schema;
    schema.setDelimiter(':');
    schema.add<int>("port");
    schema.add<std::string>("host");

    auto result = schema.parse("port: 8080\nhost: \"localhost\"");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("port").as<int>() == 8080);
    CHECK(result.get("host").as<std::string>() == "localhost");
}

TEST_CASE("Custom string delimiter", "[markers]") {
    cord::Schema schema;
    schema.setDelimiter("==");
    schema.add<int>("port");
    schema.add<bool>("debug");

    auto result = schema.parse("port == 8080\ndebug == true");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("port").as<int>() == 8080);
    CHECK(result.get("debug").as<bool>() == true);
}

TEST_CASE("Space delimiter", "[markers]") {
    cord::Schema schema;
    schema.setDelimiter(' ');
    schema.add<int>("port");
    schema.add<std::string>("host");

    auto result = schema.parse("port 8080\nhost \"localhost\"");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("port").as<int>() == 8080);
    CHECK(result.get("host").as<std::string>() == "localhost");
}

TEST_CASE("Custom char comment marker", "[markers]") {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.setCommentMarker(';');
    schema.add<int>("port");

    auto result = schema.parse("; ini-style comment\nport = 8080");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("port").as<int>() == 8080);
}

TEST_CASE("Custom string comment marker", "[markers]") {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.setCommentMarker("//");
    schema.add<int>("port");
    schema.add<bool>("debug");

    auto result = schema.parse("// c++ style\nport = 8080\ndebug = true // inline");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("port").as<int>() == 8080);
    CHECK(result.get("debug").as<bool>() == true);
}

TEST_CASE("SQL-style comment marker", "[markers]") {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.setCommentMarker("--");
    schema.add<std::string>("host");

    auto result = schema.parse("-- sql comment\nhost = \"localhost\"");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("host").as<std::string>() == "localhost");
}

TEST_CASE("Combined custom delimiter and comment marker", "[markers]") {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.setDelimiter("==");
    schema.setCommentMarker("//");
    schema.add<int>("port");
    schema.add<std::string>("host");

    auto result = schema.parse("// comment\nport == 8080\nhost == \"localhost\" // inline");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("port").as<int>() == 8080);
    CHECK(result.get("host").as<std::string>() == "localhost");
}
