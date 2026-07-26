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
    auto errors = result.getErrors();
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
    auto errors = result.getErrors();
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

TEST_CASE("contains() returns true for present key", "[schema]") {
    cord::Schema schema;
    schema.add<int>("port");
    schema.add<std::string>("host").default_("localhost");

    auto result = schema.parse("port = 8080");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.contains("port"));
    CHECK(result.contains("host")); // from default
    CHECK_FALSE(result.contains("unknown"));
}

TEST_CASE("contains() returns false for absent optional field", "[schema]") {
    cord::Schema schema;
    schema.add<int>("port");

    auto result = schema.parse("");
    REQUIRE_FALSE(result.hasErrors());
    CHECK_FALSE(result.contains("port"));
}

TEST_CASE("Case-insensitive: off by default, key not matched", "[schema][case]") {
    cord::Schema schema;
    schema.add<int>("port");

    auto result = schema.parse("PORT = 8080");
    REQUIRE_FALSE(result.hasErrors()); // lenient: no error, but key not matched
    CHECK_FALSE(result.contains("port"));
}

TEST_CASE("Case-insensitive: uppercase key matches lowercase field", "[schema][case]") {
    cord::Schema schema;
    schema.setCaseInsensitive(true);
    schema.add<int>("port");

    auto result = schema.parse("PORT = 8080");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("port").as<int>() == 8080);
}

TEST_CASE("Case-insensitive: mixed case key matches field", "[schema][case]") {
    cord::Schema schema;
    schema.setCaseInsensitive(true);
    schema.add<std::string>("host");
    schema.add<int>("port");

    auto result = schema.parse("Host = \"localhost\"\nPORT = 9090");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("host").as<std::string>() == "localhost");
    CHECK(result.get("port").as<int>() == 9090);
}

TEST_CASE("Case-insensitive: strict mode still rejects unknown keys", "[schema][case]") {
    cord::Schema schema;
    schema.setCaseInsensitive(true);
    schema.setStrict(true);
    schema.add<int>("port");

    auto result = schema.parse("PORT = 8080\nunknown = 1");
    REQUIRE(result.hasErrors());
    CHECK(result.getErrors()[0].message.find("unknown") != std::string::npos);
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

TEST_CASE("min() enforced for int", "[schema][constraints]") {
    cord::Schema schema;
    schema.add<int>("port").min(1024);

    auto ok = schema.parse("port = 8080");
    REQUIRE_FALSE(ok.hasErrors());

    auto bad = schema.parse("port = 80");
    REQUIRE(bad.hasErrors());
    CHECK(bad.getErrors()[0].message.find("below minimum") != std::string::npos);
}

TEST_CASE("max() enforced for int", "[schema][constraints]") {
    cord::Schema schema;
    schema.add<int>("port").max(65535);

    auto ok = schema.parse("port = 8080");
    REQUIRE_FALSE(ok.hasErrors());

    auto bad = schema.parse("port = 99999");
    REQUIRE(bad.hasErrors());
    CHECK(bad.getErrors()[0].message.find("exceeds maximum") != std::string::npos);
}

TEST_CASE("min() and max() combined for int", "[schema][constraints]") {
    cord::Schema schema;
    schema.add<int>("port").min(1024).max(65535);

    CHECK_FALSE(schema.parse("port = 8080").hasErrors());
    CHECK(schema.parse("port = 80").hasErrors());
    CHECK(schema.parse("port = 99999").hasErrors());
}

TEST_CASE("min() enforced for float", "[schema][constraints]") {
    cord::Schema schema;
    schema.add<float>("ratio").min(0.0f).max(1.0f);

    CHECK_FALSE(schema.parse("ratio = 0.5").hasErrors());
    CHECK(schema.parse("ratio = -0.1").hasErrors());
    CHECK(schema.parse("ratio = 1.1").hasErrors());
}

TEST_CASE("min() enforced for double", "[schema][constraints]") {
    cord::Schema schema;
    schema.add<double>("threshold").min(0.0).max(100.0);

    CHECK_FALSE(schema.parse("threshold = 50.0").hasErrors());
    CHECK(schema.parse("threshold = -1.0").hasErrors());
    CHECK(schema.parse("threshold = 101.0").hasErrors());
}

TEST_CASE("min() only, no max", "[schema][constraints]") {
    cord::Schema schema;
    schema.add<int>("workers").min(1);

    CHECK_FALSE(schema.parse("workers = 8").hasErrors());
    CHECK(schema.parse("workers = 0").hasErrors());
}

TEST_CASE("max() only, no min", "[schema][constraints]") {
    cord::Schema schema;
    schema.add<int>("retries").max(10);

    CHECK_FALSE(schema.parse("retries = 3").hasErrors());
    CHECK(schema.parse("retries = 11").hasErrors());
}

TEST_CASE("constraint error includes line number", "[schema][constraints]") {
    cord::Schema schema;
    schema.add<int>("port").min(1024);

    auto result = schema.parse("port = 80");
    REQUIRE(result.hasErrors());
    REQUIRE(result.getErrors()[0].line.has_value());
    CHECK(result.getErrors()[0].line.value() == 1);
}

TEST_CASE("oneOf() accepted value passes", "[schema][constraints]") {
    cord::Schema schema;
    schema.add<std::string>("level").oneOf({"debug", "info", "warn", "error"});

    CHECK_FALSE(schema.parse("level = \"info\"").hasErrors());
    CHECK_FALSE(schema.parse("level = \"debug\"").hasErrors());
}

TEST_CASE("oneOf() rejected value produces error", "[schema][constraints]") {
    cord::Schema schema;
    schema.add<std::string>("level").oneOf({"debug", "info", "warn", "error"});

    auto result = schema.parse("level = \"trace\"");
    REQUIRE(result.hasErrors());
    CHECK(result.getErrors()[0].message.find("not in allowed values") != std::string::npos);
}

TEST_CASE("oneOf() error message contains rejected value", "[schema][constraints]") {
    cord::Schema schema;
    schema.add<std::string>("env").oneOf({"dev", "staging", "prod"});

    auto result = schema.parse("env = \"local\"");
    REQUIRE(result.hasErrors());
    CHECK(result.getErrors()[0].message.find("\"local\"") != std::string::npos);
}

TEST_CASE("oneOf() for int field", "[schema][constraints]") {
    cord::Schema schema;
    schema.add<int>("verbosity").oneOf({0, 1, 2, 3});

    CHECK_FALSE(schema.parse("verbosity = 2").hasErrors());
    CHECK(schema.parse("verbosity = 5").hasErrors());
}

TEST_CASE("oneOf() for bool field", "[schema][constraints]") {
    cord::Schema schema;
    schema.add<bool>("enabled").oneOf({true});

    CHECK_FALSE(schema.parse("enabled = true").hasErrors());
    CHECK(schema.parse("enabled = false").hasErrors());
}

TEST_CASE("oneOf() combined with required()", "[schema][constraints]") {
    cord::Schema schema;
    schema.add<std::string>("mode").required().oneOf({"read", "write"});

    CHECK_FALSE(schema.parse("mode = \"read\"").hasErrors());

    auto missing = schema.parse("");
    REQUIRE(missing.hasErrors());
    CHECK(missing.getErrors()[0].message.find("mode") != std::string::npos);

    auto invalid = schema.parse("mode = \"exec\"");
    REQUIRE(invalid.hasErrors());
    CHECK(invalid.getErrors()[0].message.find("not in allowed values") != std::string::npos);
}

TEST_CASE("oneOf() error includes line number", "[schema][constraints]") {
    cord::Schema schema;
    schema.add<std::string>("level").oneOf({"info", "warn"});

    auto result = schema.parse("level = \"trace\"");
    REQUIRE(result.hasErrors());
    REQUIRE(result.getErrors()[0].line.has_value());
    CHECK(result.getErrors()[0].line.value() == 1);
}
