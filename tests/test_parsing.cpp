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

TEST_CASE("Float overflow rejected", "[parsing]") {
    cord::Schema schema;
    schema.add<float>("value");

    auto result = schema.parse("value = 1e9999");
    REQUIRE(result.hasErrors());
    CHECK(result.getErrors()[0].message.find("out of range for float") != std::string::npos);
}

TEST_CASE("Float non-numeric string rejected", "[parsing]") {
    cord::Schema schema;
    schema.add<float>("value");

    auto result = schema.parse("value = abc");
    REQUIRE(result.hasErrors());
    CHECK(result.getErrors()[0].message.find("not a valid float") != std::string::npos);
}

TEST_CASE("Trailing backslash in string rejected", "[parsing][escape]") {
    cord::Schema schema;
    schema.add<std::string>("val");

    auto result = schema.parse("val = \"bad\\\"");
    REQUIRE(result.hasErrors());
    CHECK(result.getErrors()[0].message.find("trailing backslash") != std::string::npos);
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
    auto errors = result.getErrors();
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

TEST_CASE("String escape sequences parsed correctly", "[parsing][escape]") {
    cord::Schema schema;
    schema.add<std::string>("a");
    schema.add<std::string>("b");
    schema.add<std::string>("c");
    schema.add<std::string>("d");

    auto result = schema.parse(R"(
a = "hello \"world\""
b = "line1\nline2"
c = "col1\tcol2"
d = "back\\slash"
)");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("a").as<std::string>() == "hello \"world\"");
    CHECK(result.get("b").as<std::string>() == "line1\nline2");
    CHECK(result.get("c").as<std::string>() == "col1\tcol2");
    CHECK(result.get("d").as<std::string>() == "back\\slash");
}

TEST_CASE("Unknown escape sequence produces error", "[parsing][escape]") {
    cord::Schema schema;
    schema.add<std::string>("val");

    auto result = schema.parse(R"(val = "bad\qescape")");
    REQUIRE(result.hasErrors());
    CHECK(result.getErrors()[0].message.find("unknown escape sequence") != std::string::npos);
}

TEST_CASE("Unquoted string produces error with reason", "[parsing][escape]") {
    cord::Schema schema;
    schema.add<std::string>("val");

    auto result = schema.parse("val = notquoted");
    REQUIRE(result.hasErrors());
    CHECK(result.getErrors()[0].message.find("quoted") != std::string::npos);
}

TEST_CASE("Escape sequences in vector strings", "[parsing][escape]") {
    cord::Schema schema;
    schema.add<std::vector<std::string>>("tags");

    auto result = schema.parse(R"(tags = ["hello\tworld", "line1\nline2", "quo\"ted"])");
    REQUIRE_FALSE(result.hasErrors());
    auto tags = result.get("tags").as<std::vector<std::string>>();
    CHECK(tags[0] == "hello\tworld");
    CHECK(tags[1] == "line1\nline2");
    CHECK(tags[2] == "quo\"ted");
}
