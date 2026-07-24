#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "cord.hpp"

TEST_CASE("Primitive parsing", "[parsing]") {
    cord::Schema schema;
    schema.add<int>("port");
    schema.add<double>("ratio");
    schema.add<float>("threshold");
    schema.add<bool>("debug");
    schema.add<std::string>("host");

    auto result = schema.parse(R"(
port = 8080
ratio = 3.14
threshold = 1.5
debug = true
host = "localhost"
)");

    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("port").as<int>() == 8080);
    CHECK(result.get("ratio").as<double>() == 3.14);
    CHECK_THAT(result.get("threshold").as<float>(), Catch::Matchers::WithinRel(1.5f, 1e-5f));
    CHECK(result.get("debug").as<bool>() == true);
    CHECK(result.get("host").as<std::string>() == "localhost");
}

TEST_CASE("Bool parsing", "[parsing]") {
    cord::Schema schema;
    schema.add<bool>("a");
    schema.add<bool>("b");

    auto result = schema.parse("a = true\nb = false");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("a").as<bool>() == true);
    CHECK(result.get("b").as<bool>() == false);
}

TEST_CASE("Invalid bool rejected", "[parsing]") {
    cord::Schema schema;
    schema.add<bool>("flag");

    auto result = schema.parse("flag = yes");
    CHECK(result.hasErrors());
}

TEST_CASE("Unquoted string rejected", "[parsing]") {
    cord::Schema schema;
    schema.add<std::string>("host");

    auto result = schema.parse("host = localhost");
    CHECK(result.hasErrors());
}

TEST_CASE("Integer overflow rejected", "[parsing]") {
    cord::Schema schema;
    schema.add<int>("value");

    auto result = schema.parse("value = 9999999999999999999");
    CHECK(result.hasErrors());
}

TEST_CASE("Double overflow rejected", "[parsing]") {
    cord::Schema schema;
    schema.add<double>("value");

    auto result = schema.parse("value = 1e9999");
    CHECK(result.hasErrors());
}

TEST_CASE("Double with trailing garbage rejected", "[parsing]") {
    cord::Schema schema;
    schema.add<double>("value");

    auto result = schema.parse("value = 1.5abc");
    CHECK(result.hasErrors());
}

TEST_CASE("Double non-numeric string rejected", "[parsing]") {
    cord::Schema schema;
    schema.add<double>("value");

    auto result = schema.parse("value = abc");
    CHECK(result.hasErrors());
}

TEST_CASE("Int non-numeric string rejected", "[parsing]") {
    cord::Schema schema;
    schema.add<int>("value");

    auto result = schema.parse("value = abc");
    CHECK(result.hasErrors());
}

TEST_CASE("Whitespace trimming", "[parsing]") {
    cord::Schema schema;
    schema.add<int>("port");
    schema.add<std::string>("host");

    auto result = schema.parse("   port   =   8080\nhost=\"localhost\"");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("port").as<int>() == 8080);
    CHECK(result.get("host").as<std::string>() == "localhost");
}

TEST_CASE("Empty lines skipped", "[parsing]") {
    cord::Schema schema;
    schema.add<int>("port");
    schema.add<bool>("debug");

    auto result = schema.parse("\n\nport = 8080\n\n\ndebug = true\n\n");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("port").as<int>() == 8080);
    CHECK(result.get("debug").as<bool>() == true);
}

TEST_CASE("Missing delimiter error", "[parsing]") {
    cord::Schema schema;
    schema.add<int>("port");

    auto result = schema.parse("port = 8080\nthis_has_no_equals");
    REQUIRE(result.hasErrors());
    auto& errors = result.getErrors();
    REQUIRE_FALSE(errors.empty());
    CHECK(errors[0].message.find("delimiter") != std::string::npos);
}

TEST_CASE("Duplicate key: last value wins", "[parsing]") {
    cord::Schema schema;
    schema.add<int>("port");

    auto result = schema.parse("port = 3000\nport = 8080");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("port").as<int>() == 8080);
}

TEST_CASE("Type mismatch throws on as<T>", "[parsing]") {
    cord::Schema schema;
    schema.add<int>("port");

    auto result = schema.parse("port = 8080");
    REQUIRE_FALSE(result.hasErrors());
    CHECK_THROWS_AS(result.get("port").as<std::string>(), cord::CordException);
}

TEST_CASE("get() throws on missing key", "[parsing]") {
    cord::Schema schema;
    schema.add<int>("port");

    auto result = schema.parse("");
    CHECK_THROWS_AS(result.get("port"), cord::CordException);
}
