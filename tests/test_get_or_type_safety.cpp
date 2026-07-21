// Test that get_or() rejects unsupported types at compile time
// This file should NOT compile successfully

#ifdef USE_SINGLE_HEADER
#include "cord.hpp"
#else
#include "../src/cord.hpp"
#endif

int main() {
    cord::Schema schema;
    schema.add<int>("port");

    std::string config = "";
    auto result = schema.parse(config);

    // This should fail with clear error message about unsupported type
    char fallback = 'x';
    auto value = result.get_or("port", fallback);

    return 0;
}
