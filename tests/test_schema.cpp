#include <catch2/catch_test_macros.hpp>
#include "cord.hpp"

TEST_CASE("Default values applied when key absent", "[schema]") {
    cord::Schema schema;
    schema.add<int>("port").default_(3000);
    schema.add<std::string>("host").default_("127.0.0.1");

    auto result = schema.parse("");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("port").as<int>() == 3000);
    CHECK(result.get("host").as<std::string>() == "127.0.0.1");
}

TEST_CASE("Default overridden by config value", "[schema]") {
    cord::Schema schema;
    schema.add<int>("port").default_(3000);
    schema.add<bool>("debug").default_(false);

    auto result = schema.parse("port = 8080\ndebug = true");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("port").as<int>() == 8080);
    CHECK(result.get("debug").as<bool>() == true);
}

TEST_CASE("Required field missing produces error", "[schema]") {
    cord::Schema schema;
    schema.add<int>("port").required();
    schema.add<std::string>("host").default_("localhost");

    auto result = schema.parse("host = \"example.com\"");
    REQUIRE(result.hasErrors());
    auto& errors = result.getErrors();
    REQUIRE_FALSE(errors.empty());
    CHECK(errors[0].message.find("port") != std::string::npos);
}

TEST_CASE("Required field present: no error", "[schema]") {
    cord::Schema schema;
    schema.add<int>("port").required();

    auto result = schema.parse("port = 8080");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("port").as<int>() == 8080);
}

TEST_CASE("required() then default_() throws", "[schema]") {
    cord::Schema schema;
    CHECK_THROWS_AS(
        schema.add<int>("port").required().default_(3000),
        cord::CordException
    );
}

TEST_CASE("default_() then required() throws", "[schema]") {
    cord::Schema schema;
    CHECK_THROWS_AS(
        schema.add<int>("port").default_(3000).required(),
        cord::CordException
    );
}

TEST_CASE("Strict mode rejects unknown keys", "[schema]") {
    cord::Schema schema;
    schema.setStrict(true);
    schema.add<int>("port");

    auto result = schema.parse("port = 8080\nunknown_key = 123");
    REQUIRE(result.hasErrors());
    auto& errors = result.getErrors();
    REQUIRE_FALSE(errors.empty());
    CHECK(errors[0].message.find("unknown_key") != std::string::npos);
}

TEST_CASE("Lenient mode ignores unknown keys", "[schema]") {
    cord::Schema schema;
    schema.setStrict(false);
    schema.add<int>("port");

    auto result = schema.parse("port = 8080\nunknown_key = 123\nanother = \"value\"");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("port").as<int>() == 8080);
}

TEST_CASE("Empty delimiter throws", "[schema]") {
    cord::Schema schema;
    CHECK_THROWS_AS(schema.setDelimiter(""), cord::CordException);
}

TEST_CASE("Empty comment marker throws", "[schema]") {
    cord::Schema schema;
    CHECK_THROWS_AS(schema.setCommentMarker(""), cord::CordException);
}
