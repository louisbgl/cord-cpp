#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "cord.hpp"

TEST_CASE("Vector<int> parsing", "[vectors]") {
    cord::Schema schema;
    schema.add<std::vector<int>>("ports");

    auto result = schema.parse("ports = [8080, 8081, 8082]");
    REQUIRE_FALSE(result.hasErrors());
    auto ports = result.get("ports").as<std::vector<int>>();
    REQUIRE(ports.size() == 3);
    CHECK(ports[0] == 8080);
    CHECK(ports[1] == 8081);
    CHECK(ports[2] == 8082);
}

TEST_CASE("Vector<string> parsing", "[vectors]") {
    cord::Schema schema;
    schema.add<std::vector<std::string>>("hosts");

    auto result = schema.parse("hosts = [\"server1\", \"server2\", \"server3\"]");
    REQUIRE_FALSE(result.hasErrors());
    auto hosts = result.get("hosts").as<std::vector<std::string>>();
    REQUIRE(hosts.size() == 3);
    CHECK(hosts[0] == "server1");
    CHECK(hosts[1] == "server2");
    CHECK(hosts[2] == "server3");
}

TEST_CASE("Vector<bool> parsing", "[vectors]") {
    cord::Schema schema;
    schema.add<std::vector<bool>>("flags");

    auto result = schema.parse("flags = [true, false, true]");
    REQUIRE_FALSE(result.hasErrors());
    auto flags = result.get("flags").as<std::vector<bool>>();
    REQUIRE(flags.size() == 3);
    CHECK(flags[0] == true);
    CHECK(flags[1] == false);
    CHECK(flags[2] == true);
}

TEST_CASE("Vector<float> parsing", "[vectors]") {
    cord::Schema schema;
    schema.add<std::vector<float>>("ratios");

    auto result = schema.parse("ratios = [1.5, 2.5, 3.5]");
    REQUIRE_FALSE(result.hasErrors());
    auto ratios = result.get("ratios").as<std::vector<float>>();
    REQUIRE(ratios.size() == 3);
    CHECK_THAT(ratios[0], Catch::Matchers::WithinRel(1.5f, 1e-5f));
    CHECK_THAT(ratios[1], Catch::Matchers::WithinRel(2.5f, 1e-5f));
    CHECK_THAT(ratios[2], Catch::Matchers::WithinRel(3.5f, 1e-5f));
}

TEST_CASE("Vector<double> parsing", "[vectors]") {
    cord::Schema schema;
    schema.add<std::vector<double>>("values");

    auto result = schema.parse("values = [3.14159, 2.71828, 1.41421]");
    REQUIRE_FALSE(result.hasErrors());
    auto values = result.get("values").as<std::vector<double>>();
    REQUIRE(values.size() == 3);
    CHECK_THAT(values[0], Catch::Matchers::WithinRel(3.14159, 1e-5));
    CHECK_THAT(values[1], Catch::Matchers::WithinRel(2.71828, 1e-5));
    CHECK_THAT(values[2], Catch::Matchers::WithinRel(1.41421, 1e-5));
}

TEST_CASE("Empty vectors", "[vectors]") {
    cord::Schema schema;
    schema.add<std::vector<int>>("ports");
    schema.add<std::vector<std::string>>("hosts");

    auto result = schema.parse("ports = []\nhosts = []");
    REQUIRE_FALSE(result.hasErrors());
    CHECK(result.get("ports").as<std::vector<int>>().empty());
    CHECK(result.get("hosts").as<std::vector<std::string>>().empty());
}

TEST_CASE("Single-element vector", "[vectors]") {
    cord::Schema schema;
    schema.add<std::vector<int>>("port");

    auto result = schema.parse("port = [8080]");
    REQUIRE_FALSE(result.hasErrors());
    auto port = result.get("port").as<std::vector<int>>();
    REQUIRE(port.size() == 1);
    CHECK(port[0] == 8080);
}

TEST_CASE("Vector with extra whitespace", "[vectors]") {
    cord::Schema schema;
    schema.add<std::vector<int>>("ports");

    auto result = schema.parse("ports = [ 8080 , 8081 , 8082 ]");
    REQUIRE_FALSE(result.hasErrors());
    auto ports = result.get("ports").as<std::vector<int>>();
    REQUIRE(ports.size() == 3);
    CHECK(ports[0] == 8080);
    CHECK(ports[1] == 8081);
    CHECK(ports[2] == 8082);
}

TEST_CASE("Vector with default", "[vectors]") {
    cord::Schema schema;
    schema.add<std::vector<int>>("ports").default_(std::vector<int>{8080, 8081});

    auto result = schema.parse("");
    REQUIRE_FALSE(result.hasErrors());
    auto ports = result.get("ports").as<std::vector<int>>();
    REQUIRE(ports.size() == 2);
    CHECK(ports[0] == 8080);
    CHECK(ports[1] == 8081);
}

TEST_CASE("Vector missing brackets rejected", "[vectors]") {
    cord::Schema schema;
    schema.add<std::vector<int>>("ports");

    auto result = schema.parse("ports = 8080, 8081, 8082");
    CHECK(result.hasErrors());
}

TEST_CASE("Vector missing closing bracket rejected", "[vectors]") {
    cord::Schema schema;
    schema.add<std::vector<int>>("ports");

    auto result = schema.parse("ports = [8080, 8081, 8082");
    CHECK(result.hasErrors());
}

TEST_CASE("Vector with unquoted strings rejected", "[vectors]") {
    cord::Schema schema;
    schema.add<std::vector<std::string>>("hosts");

    auto result = schema.parse("hosts = [server1, server2]");
    CHECK(result.hasErrors());
}
