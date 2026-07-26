#include <catch2/catch_test_macros.hpp>
#include "cord.hpp"

TEST_CASE("getErrors returns structured error info", "[errors]") {
    cord::Schema schema;
    schema.add<int>("port");

    auto result = schema.parse("port = \"not_a_number\"");
    REQUIRE(result.hasErrors());

    auto errors = result.getErrors();
    REQUIRE_FALSE(errors.empty());
    CHECK_FALSE(errors[0].message.empty());
}

TEST_CASE("Error includes line number", "[errors]") {
    cord::Schema schema;
    schema.add<int>("port");

    auto result = schema.parse("port = 8080\nbad_line_no_delimiter");
    REQUIRE(result.hasErrors());

    auto errors = result.getErrors();
    REQUIRE_FALSE(errors.empty());
    REQUIRE(errors[0].line.has_value());
    CHECK(errors[0].line.value() == 2);
}

TEST_CASE("Multiple errors accumulated", "[errors]") {
    cord::Schema schema;
    schema.setStrict(true);
    schema.add<int>("port").required();

    auto result = schema.parse("unknown1 = 1\nunknown2 = 2");
    REQUIRE(result.hasErrors());
    CHECK(result.getErrors().size() >= 2);
}

TEST_CASE("No errors on valid input", "[errors]") {
    cord::Schema schema;
    schema.add<int>("port").default_(8080);

    auto result = schema.parse("");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.getErrors().empty());
}

TEST_CASE("CordException::what() returns message", "[errors]") {
    cord::CordException ex("something went wrong");
    CHECK(std::string(ex.what()) == "[CORD] something went wrong");
}

TEST_CASE("CordException with file and line formats correctly", "[errors]") {
    cord::CordException ex("myfile.cpp", 42, "something went wrong");
    std::string msg = ex.what();
    CHECK(msg.find("[CORD]") != std::string::npos);
    CHECK(msg.find("myfile.cpp") != std::string::npos);
    CHECK(msg.find("42") != std::string::npos);
    CHECK(msg.find("something went wrong") != std::string::npos);
}

TEST_CASE("Multiple missing required fields consolidated into one error", "[errors]") {
    cord::Schema schema;
    schema.add<int>("port").required();
    schema.add<std::string>("host").required();
    schema.add<bool>("debug").required();

    auto result = schema.parse("");
    REQUIRE(result.hasErrors());
    auto errors = result.getErrors();
    REQUIRE(errors.size() == 1);
    CHECK(errors[0].message.find("port") != std::string::npos);
    CHECK(errors[0].message.find("host") != std::string::npos);
    CHECK(errors[0].message.find("debug") != std::string::npos);
}

TEST_CASE("Required fields error printed before parse errors", "[errors]") {
    cord::Schema schema;
    schema.setStrict(true);
    schema.add<int>("port").required();

    auto result = schema.parse("unknown = 1");
    REQUIRE(result.hasErrors());
    auto errors = result.getErrors();
    REQUIRE(errors.size() >= 2);
    CHECK(errors[0].message.find("port") != std::string::npos);  // required first
}

TEST_CASE("Unknown key in strict mode includes line number", "[errors]") {
    cord::Schema schema;
    schema.setStrict(true);
    schema.add<int>("port");

    auto result = schema.parse("port = 8080\nbad_key = 1");
    REQUIRE(result.hasErrors());
    auto errors = result.getErrors();
    REQUIRE(errors[0].line.has_value());
    CHECK(errors[0].line.value() == 2);
}
