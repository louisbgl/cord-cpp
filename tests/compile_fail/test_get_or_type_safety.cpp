// This file must NOT compile — verifies get_or() rejects unsupported types at compile time
#include "cord.hpp"

int main() {
    cord::Schema schema;
    schema.add<int>("port");

    auto result = schema.parse("");
    char fallback = 'x';
    auto value = result.get_or("port", fallback); // static_assert should fire here
    return 0;
}
