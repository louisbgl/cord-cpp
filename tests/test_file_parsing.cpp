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
    auto& errors = result.getErrors();
    REQUIRE_FALSE(errors.empty());
    CHECK(errors[0].message.find("Failed to open file") != std::string::npos);
}
