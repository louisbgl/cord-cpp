#ifdef USE_SINGLE_HEADER
#include "cord.hpp"
#else
#include "../src/cord.hpp"
#endif

#include <cassert>
#include <iostream>

void test_get_or_key_exists() {
    cord::Schema schema;
    schema.add<int>("port");
    schema.add<std::string>("host");

    std::string config = R"(
port = 8080
host = "localhost"
)";

    auto result = schema.parse(config);
    assert(!result.hasErrors());

    int port = result.get_or("port", 3000).as<int>();
    assert(port == 8080);

    std::string host = result.get_or("host", "default").as<std::string>();
    assert(host == "localhost");

    std::cout << "✓ test_get_or_key_exists passed\n";
}

void test_get_or_key_missing() {
    cord::Schema schema;
    schema.add<int>("port");
    schema.add<std::string>("host");

    std::string config = "";

    auto result = schema.parse(config);
    assert(!result.hasErrors());

    int port = result.get_or("port", 3000).as<int>();
    assert(port == 3000);

    std::string host = result.get_or("host", "fallback").as<std::string>();
    assert(host == "fallback");

    std::cout << "✓ test_get_or_key_missing passed\n";
}

void test_get_or_context_dependent() {
    cord::Schema schema;
    schema.add<int>("port");
    schema.add<std::string>("env").required();

    std::string config = R"(
env = "prod"
)";

    auto result = schema.parse(config);
    assert(!result.hasErrors());

    auto env = result.get("env").as<std::string>();
    int port = result.get_or("port", env == "prod" ? 443 : 8080).as<int>();
    assert(port == 443);

    std::cout << "✓ test_get_or_context_dependent passed\n";
}

void test_get_or_with_bool() {
    cord::Schema schema;
    schema.add<bool>("debug");

    std::string config = "";

    auto result = schema.parse(config);
    assert(!result.hasErrors());

    bool debug = result.get_or("debug", false).as<bool>();
    assert(debug == false);

    std::cout << "✓ test_get_or_with_bool passed\n";
}

void test_get_or_with_float() {
    cord::Schema schema;
    schema.add<float>("ratio");

    std::string config = "";

    auto result = schema.parse(config);
    assert(!result.hasErrors());

    float ratio = result.get_or("ratio", 1.5f).as<float>();
    assert(ratio == 1.5f);

    std::cout << "✓ test_get_or_with_float passed\n";
}

void test_get_or_with_double() {
    cord::Schema schema;
    schema.add<double>("pi");

    std::string config = "";

    auto result = schema.parse(config);
    assert(!result.hasErrors());

    double pi = result.get_or("pi", 3.14159).as<double>();
    assert(pi == 3.14159);

    std::cout << "✓ test_get_or_with_double passed\n";
}

void test_get_or_with_vector() {
    cord::Schema schema;
    schema.add<std::vector<int>>("ports");

    std::string config = "";

    auto result = schema.parse(config);
    assert(!result.hasErrors());

    auto ports = result.get_or("ports", std::vector<int>{8080, 8081}).as<std::vector<int>>();
    assert(ports.size() == 2);
    assert(ports[0] == 8080);
    assert(ports[1] == 8081);

    std::cout << "✓ test_get_or_with_vector passed\n";
}

void test_get_or_overrides_schema_default() {
    cord::Schema schema;
    schema.add<int>("port").default_(9000);

    std::string config = "";

    auto result = schema.parse(config);
    assert(!result.hasErrors());

    int port_default = result.get("port").as<int>();
    assert(port_default == 9000);

    int port_override = result.get_or("port", 3000).as<int>();
    assert(port_override == 9000);

    std::cout << "✓ test_get_or_overrides_schema_default passed\n";
}

int main() {
    test_get_or_key_exists();
    test_get_or_key_missing();
    test_get_or_context_dependent();
    test_get_or_with_bool();
    test_get_or_with_float();
    test_get_or_with_double();
    test_get_or_with_vector();
    test_get_or_overrides_schema_default();

    return 0;
}
