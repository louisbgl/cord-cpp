#include <catch2/catch_test_macros.hpp>
#include "cord.hpp"

TEST_CASE("get_or returns parsed value when key present", "[get_or]") {
    cord::Schema schema;
    schema.add<int>("port");
    schema.add<std::string>("host");

    auto result = schema.parse("port = 8080\nhost = \"localhost\"");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get_or("port", 3000).as<int>() == 8080);
    CHECK(result.get_or("host", "default").as<std::string>() == "localhost");
}

TEST_CASE("get_or returns fallback when key absent", "[get_or]") {
    cord::Schema schema;
    schema.add<int>("port");
    schema.add<std::string>("host");
    schema.add<bool>("debug");
    schema.add<float>("ratio");
    schema.add<double>("pi");

    auto result = schema.parse("");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get_or("port", 3000).as<int>() == 3000);
    CHECK(result.get_or("host", "fallback").as<std::string>() == "fallback");
    CHECK(result.get_or("debug", false).as<bool>() == false);
    CHECK(result.get_or("ratio", 1.5f).as<float>() == 1.5f);
    CHECK(result.get_or("pi", 3.14).as<double>() == 3.14);
}

TEST_CASE("get_or with schema default: schema default wins over get_or fallback", "[get_or]") {
    cord::Schema schema;
    schema.add<int>("port").default_(9000);

    auto result = schema.parse("");
    REQUIRE_FALSE(result.hasErrors());
    // key is in result (from schema default), so get_or returns schema default
    CHECK(result.get_or("port", 3000).as<int>() == 9000);
}

TEST_CASE("get_or with runtime-computed fallback", "[get_or]") {
    cord::Schema schema;
    schema.add<int>("port");
    schema.add<std::string>("env").required();

    auto result = schema.parse("env = \"prod\"");
    REQUIRE_FALSE(result.hasErrors());

    auto env = result.get("env").as<std::string>();
    int port = result.get_or("port", env == "prod" ? 443 : 8080).as<int>();
    CHECK(port == 443);
}

TEST_CASE("get_or with vector fallback", "[get_or]") {
    cord::Schema schema;
    schema.add<std::vector<int>>("ports");

    auto result = schema.parse("");
    REQUIRE_FALSE(result.hasErrors());

    auto ports = result.get_or("ports", std::vector<int>{8080, 8081}).as<std::vector<int>>();
    REQUIRE(ports.size() == 2);
    CHECK(ports[0] == 8080);
    CHECK(ports[1] == 8081);
}
