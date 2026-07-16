#ifdef USE_SINGLE_HEADER
#include "cord.hpp"
#else
#include "../src/cord.hpp"
#endif

#include <cassert>
#include <iostream>
#include <cmath>

void test_vector_int() {
    cord::Schema schema;
    schema.add<std::vector<int>>("ports");

    std::string config = "ports = [8080, 8081, 8082]";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    auto ports = result.get("ports").as<std::vector<int>>();
    assert(ports.size() == 3);
    assert(ports[0] == 8080);
    assert(ports[1] == 8081);
    assert(ports[2] == 8082);

    std::cout << "✓ test_vector_int passed\n";
}

void test_vector_string() {
    cord::Schema schema;
    schema.add<std::vector<std::string>>("hosts");

    std::string config = "hosts = [\"server1\", \"server2\", \"server3\"]";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    auto hosts = result.get("hosts").as<std::vector<std::string>>();
    assert(hosts.size() == 3);
    assert(hosts[0] == "server1");
    assert(hosts[1] == "server2");
    assert(hosts[2] == "server3");

    std::cout << "✓ test_vector_string passed\n";
}

void test_vector_bool() {
    cord::Schema schema;
    schema.add<std::vector<bool>>("flags");

    std::string config = "flags = [true, false, true]";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    auto flags = result.get("flags").as<std::vector<bool>>();
    assert(flags.size() == 3);
    assert(flags[0] == true);
    assert(flags[1] == false);
    assert(flags[2] == true);

    std::cout << "✓ test_vector_bool passed\n";
}

void test_vector_float() {
    cord::Schema schema;
    schema.add<std::vector<float>>("ratios");

    std::string config = "ratios = [1.5, 2.5, 3.5]";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    auto ratios = result.get("ratios").as<std::vector<float>>();
    assert(ratios.size() == 3);
    assert(std::abs(ratios[0] - 1.5f) < 0.001f);
    assert(std::abs(ratios[1] - 2.5f) < 0.001f);
    assert(std::abs(ratios[2] - 3.5f) < 0.001f);

    std::cout << "✓ test_vector_float passed\n";
}

void test_vector_double() {
    cord::Schema schema;
    schema.add<std::vector<double>>("values");

    std::string config = "values = [3.14159, 2.71828, 1.41421]";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    auto values = result.get("values").as<std::vector<double>>();
    assert(values.size() == 3);
    assert(std::abs(values[0] - 3.14159) < 0.00001);
    assert(std::abs(values[1] - 2.71828) < 0.00001);
    assert(std::abs(values[2] - 1.41421) < 0.00001);

    std::cout << "✓ test_vector_double passed\n";
}

void test_vector_empty() {
    cord::Schema schema;
    schema.add<std::vector<int>>("ports");
    schema.add<std::vector<std::string>>("hosts");

    std::string config = R"(
ports = []
hosts = []
)";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    auto ports = result.get("ports").as<std::vector<int>>();
    auto hosts = result.get("hosts").as<std::vector<std::string>>();
    assert(ports.empty());
    assert(hosts.empty());

    std::cout << "✓ test_vector_empty passed\n";
}

void test_vector_single_element() {
    cord::Schema schema;
    schema.add<std::vector<int>>("port");

    std::string config = "port = [8080]";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    auto port = result.get("port").as<std::vector<int>>();
    assert(port.size() == 1);
    assert(port[0] == 8080);

    std::cout << "✓ test_vector_single_element passed\n";
}

void test_vector_whitespace() {
    cord::Schema schema;
    schema.add<std::vector<int>>("ports");

    std::string config = "ports = [ 8080 , 8081 , 8082 ]";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    auto ports = result.get("ports").as<std::vector<int>>();
    assert(ports.size() == 3);
    assert(ports[0] == 8080);
    assert(ports[1] == 8081);
    assert(ports[2] == 8082);

    std::cout << "✓ test_vector_whitespace passed\n";
}

void test_vector_missing_brackets() {
    cord::Schema schema;
    schema.add<std::vector<int>>("ports");

    std::string config = "ports = 8080, 8081, 8082";

    auto result = schema.parse(config);
    assert(result.hasErrors());

    std::cout << "✓ test_vector_missing_brackets passed\n";
}

void test_vector_unquoted_strings() {
    cord::Schema schema;
    schema.add<std::vector<std::string>>("hosts");

    std::string config = "hosts = [server1, server2]";

    auto result = schema.parse(config);
    assert(result.hasErrors());

    std::cout << "✓ test_vector_unquoted_strings passed\n";
}

void test_vector_default() {
    cord::Schema schema;
    schema.add<std::vector<int>>("ports").default_(std::vector<int>{8080, 8081});

    std::string config = "";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    auto ports = result.get("ports").as<std::vector<int>>();
    assert(ports.size() == 2);
    assert(ports[0] == 8080);
    assert(ports[1] == 8081);

    std::cout << "✓ test_vector_default passed\n";
}

int main() {
    test_vector_int();
    test_vector_string();
    test_vector_bool();
    test_vector_float();
    test_vector_double();
    test_vector_empty();
    test_vector_single_element();
    test_vector_whitespace();
    test_vector_missing_brackets();
    test_vector_unquoted_strings();
    test_vector_default();

    return 0;
}
