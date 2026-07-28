#include <catch2/catch_test_macros.hpp>
#include "cord.hpp"
#include <fstream>
#include <cstdio>

static const std::string TMP_FILE = "cord_test_tmp.conf";

static void writeTmp(const std::string& content) {
    std::ofstream out(TMP_FILE);
    out << content;
}

TEST_CASE("parseFile reads valid config", "[file]") {
    writeTmp("port = 8080\nhost = \"localhost\"\ndebug = true\n");

    cord::Schema schema;
    schema.add<int>("port");
    schema.add<std::string>("host");
    schema.add<bool>("debug");

    auto result = schema.parseFile(TMP_FILE);
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("port").as<int>() == 8080);
    CHECK(result.get("host").as<std::string>() == "localhost");
    CHECK(result.get("debug").as<bool>() == true);

    std::remove(TMP_FILE.c_str());
}

TEST_CASE("parseFile missing file produces error", "[file]") {
    cord::Schema schema;
    schema.add<int>("port");

    auto result = schema.parseFile("nonexistent_cord_test.conf");
    REQUIRE(result.hasErrors());
    auto errors = result.getErrors();
    REQUIRE_FALSE(errors.empty());
    CHECK(errors[0].message.find("Failed to open file") != std::string::npos);
}

TEST_CASE("write() serializes parsed values", "[file][write]") {
    cord::Schema schema;
    schema.add<int>("port");
    schema.add<std::string>("host");
    schema.add<bool>("debug");

    auto result = schema.parse("port = 8080\nhost = \"localhost\"\ndebug = true");
    REQUIRE_FALSE(result.hasErrors());

    std::string out = result.write();
    CHECK(out.find("port=8080") != std::string::npos);
    CHECK(out.find("host=\"localhost\"") != std::string::npos);
    CHECK(out.find("debug=true") != std::string::npos);
}

TEST_CASE("write() reflects set() modifications", "[file][write]") {
    cord::Schema schema;
    schema.add<int>("port");
    schema.add<std::string>("host");

    auto result = schema.parse("port = 8080\nhost = \"localhost\"");
    REQUIRE_FALSE(result.hasErrors());

    result.set("port", 9090);
    std::string out = result.write();
    CHECK(out.find("port=9090") != std::string::npos);
    CHECK(out.find("port=8080") == std::string::npos);
}

TEST_CASE("write() preserves insertion order", "[file][write]") {
    cord::Schema schema;
    schema.add<int>("a");
    schema.add<int>("b");
    schema.add<int>("c");

    auto result = schema.parse("a = 1\nb = 2\nc = 3");
    REQUIRE_FALSE(result.hasErrors());

    std::string out = result.write();
    CHECK(out.find("a=1") < out.find("b=2"));
    CHECK(out.find("b=2") < out.find("c=3"));
}

TEST_CASE("writeFile() writes to disk and content is correct", "[file][write]") {
    cord::Schema schema;
    schema.add<int>("port");
    schema.add<std::string>("host");

    auto result = schema.parse("port = 8080\nhost = \"localhost\"");
    REQUIRE_FALSE(result.hasErrors());

    result.writeFile(TMP_FILE);

    std::ifstream in(TMP_FILE);
    REQUIRE(in.is_open());
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    CHECK(content.find("port=8080") != std::string::npos);
    CHECK(content.find("host=\"localhost\"") != std::string::npos);

    std::remove(TMP_FILE.c_str());
}

TEST_CASE("writeFile() throws on unwritable path", "[file][write]") {
    cord::Schema schema;
    schema.add<int>("port");

    auto result = schema.parse("port = 8080");
    REQUIRE_FALSE(result.hasErrors());

    CHECK_THROWS_AS(result.writeFile("/nonexistent_dir/out.conf"), cord::CordException);
}

TEST_CASE("set() adds new key not in original parse", "[file][write]") {
    cord::Schema schema;
    schema.add<int>("port");

    auto result = schema.parse("port = 8080");
    REQUIRE_FALSE(result.hasErrors());

    result.set("extra", 42);
    CHECK(result.contains("extra"));
    CHECK(result.get("extra").as<int>() == 42);
}

TEST_CASE("write() preserves float precision (no trailing zeros)", "[file][write]") {
    cord::Schema schema;
    schema.add<float>("ratio");
    schema.add<double>("scale");

    auto result = schema.parse("ratio = 1.5\nscale = 3.14");
    REQUIRE_FALSE(result.hasErrors());

    std::string out = result.write();
    // std::to_string produces "1.500000" / "3.140000" — check no trailing zeros
    CHECK(out.find("ratio=1.5\n") != std::string::npos);
    CHECK(out.find("scale=3.14\n") != std::string::npos);
}
