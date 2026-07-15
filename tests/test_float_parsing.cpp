#ifdef USE_SINGLE_HEADER
#include "cord.hpp"
#else
#include "../src/cord.hpp"
#endif

#include <cassert>
#include <iostream>
#include <cmath>

void test_float_basic() {
    cord::Schema schema;
    schema.add<float>("pi");
    schema.add<float>("ratio");

    std::string config = R"(
pi = 3.14159
ratio = 2.5
)";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    assert(std::abs(result.get("pi").as<float>() - 3.14159f) < 0.00001f);
    assert(std::abs(result.get("ratio").as<float>() - 2.5f) < 0.00001f);

    std::cout << "✓ test_float_basic passed\n";
}

void test_float_default() {
    cord::Schema schema;
    schema.add<float>("threshold").default_(0.5f);

    std::string config = "";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    assert(std::abs(result.get("threshold").as<float>() - 0.5f) < 0.00001f);

    std::cout << "✓ test_float_default passed\n";
}

void test_float_required() {
    cord::Schema schema;
    schema.add<float>("value").required();

    std::string config = "";

    auto result = schema.parse(config);
    assert(result.hasErrors());

    std::cout << "✓ test_float_required passed\n";
}

void test_float_override() {
    cord::Schema schema;
    schema.add<float>("multiplier").default_(1.0f);

    std::string config = "multiplier = 2.5";

    auto result = schema.parse(config);
    assert(!result.hasErrors());
    assert(std::abs(result.get("multiplier").as<float>() - 2.5f) < 0.00001f);

    std::cout << "✓ test_float_override passed\n";
}

int main() {
    test_float_basic();
    test_float_default();
    test_float_required();
    test_float_override();

    return 0;
}
