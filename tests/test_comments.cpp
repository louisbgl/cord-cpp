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

TEST_CASE("Escaped quote inside string not mistaken for end of string when stripping inline comment", "[comments]") {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.add<std::string>("msg");

    // The \" inside the string should not flip the in_quotes flag,
    // so the # after it must be treated as part of the value, not a comment
    auto result = schema.parse(R"(msg = "say \"hi\" # not a comment")");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("msg").as<std::string>() == "say \"hi\" # not a comment");
}

TEST_CASE("Single escaped quote before hash: inline comment stripper misidentifies comment start", "[comments]") {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.add<std::string>("msg");

    // _removeInlineComment toggles in_quotes on every '"', including escaped ones.
    // "it's a \"test" has three '"' chars: open, escaped, close.
    // After open: in_quotes=true. After escaped \": in_quotes=false (wrong).
    // After closing ": in_quotes=true (wrong). The # is then seen outside quotes → stripped.
    auto result = schema.parse(R"(msg = "it's a \"test" # not a comment)");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("msg").as<std::string>() == "it's a \"test");
}

TEST_CASE("Comma inside quoted string not split as vector element", "[vectors]") {
    cord::Schema schema;
    schema.add<std::vector<std::string>>("tags");

    auto result = schema.parse(R"(tags = ["hello, world", "foo"])");
    REQUIRE_FALSE(result.hasErrors());
    auto tags = result.get("tags").as<std::vector<std::string>>();
    REQUIRE(tags.size() == 2);
    CHECK(tags[0] == "hello, world");
    CHECK(tags[1] == "foo");
}
