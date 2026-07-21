#ifdef USE_SINGLE_HEADER
#include "cord.hpp"
#else
#include "../src/cord.hpp"
#endif

#include <cassert>
#include <iostream>

void test_custom_delimiter_char() {
    cord::Schema schema;
    schema.setDelimiter(':');
    schema.add<int>("port");
    schema.add<std::string>("host");

    std::string config = R"(
port: 8080
host: "localhost"
)";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    assert(result.get("port").as<int>() == 8080);
    assert(result.get("host").as<std::string>() == "localhost");

    std::cout << "✓ test_custom_delimiter_char passed\n";
}

void test_custom_delimiter_string() {
    cord::Schema schema;
    schema.setDelimiter("==");
    schema.add<int>("port");
    schema.add<bool>("debug");

    std::string config = R"(
port == 8080
debug == true
)";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    assert(result.get("port").as<int>() == 8080);
    assert(result.get("debug").as<bool>() == true);

    std::cout << "✓ test_custom_delimiter_string passed\n";
}

void test_space_delimiter() {
    cord::Schema schema;
    schema.setDelimiter(' ');
    schema.add<int>("port");
    schema.add<std::string>("host");

    std::string config = R"(
port 8080
host "localhost"
)";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    assert(result.get("port").as<int>() == 8080);
    assert(result.get("host").as<std::string>() == "localhost");

    std::cout << "✓ test_space_delimiter passed\n";
}

void test_custom_comment_marker_char() {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.setCommentMarker(';');
    schema.add<int>("port");

    std::string config = R"(
; INI-style comment
port = 8080
)";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    assert(result.get("port").as<int>() == 8080);

    std::cout << "✓ test_custom_comment_marker_char passed\n";
}

void test_custom_comment_marker_string() {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.setCommentMarker("//");
    schema.add<int>("port");
    schema.add<bool>("debug");

    std::string config = R"(
// C++-style comment
port = 8080
debug = true // inline comment
)";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    assert(result.get("port").as<int>() == 8080);
    assert(result.get("debug").as<bool>() == true);

    std::cout << "✓ test_custom_comment_marker_string passed\n";
}

void test_sql_style_comments() {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.setCommentMarker("--");
    schema.add<std::string>("host");

    std::string config = R"(
-- SQL-style comment
host = "localhost"
)";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    assert(result.get("host").as<std::string>() == "localhost");

    std::cout << "✓ test_sql_style_comments passed\n";
}

void test_combined_custom_markers() {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.setDelimiter("==");
    schema.setCommentMarker("//");
    schema.add<int>("port");
    schema.add<std::string>("host");

    std::string config = R"(
// Custom delimiter and comment marker
port == 8080
host == "localhost" // inline comment
)";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    assert(result.get("port").as<int>() == 8080);
    assert(result.get("host").as<std::string>() == "localhost");

    std::cout << "✓ test_combined_custom_markers passed\n";
}

void test_empty_delimiter_error() {
    cord::Schema schema;
    bool threw = false;
    try {
        schema.setDelimiter("");
    } catch (const cord::CordException&) {
        threw = true;
    }
    assert(threw);

    std::cout << "✓ test_empty_delimiter_error passed\n";
}

void test_empty_comment_marker_error() {
    cord::Schema schema;
    bool threw = false;
    try {
        schema.setCommentMarker("");
    } catch (const cord::CordException&) {
        threw = true;
    }
    assert(threw);

    std::cout << "✓ test_empty_comment_marker_error passed\n";
}

int main() {
    test_custom_delimiter_char();
    test_custom_delimiter_string();
    test_space_delimiter();
    test_custom_comment_marker_char();
    test_custom_comment_marker_string();
    test_sql_style_comments();
    test_combined_custom_markers();
    test_empty_delimiter_error();
    test_empty_comment_marker_error();

    return 0;
}
