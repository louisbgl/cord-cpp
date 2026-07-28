// This file must NOT compile — verifies result.set() rejects unsupported types
#include "cord.hpp"

int main() {
    cord::Schema schema;
    schema.add<int>("port");

    auto result = schema.parse("port = 8080");
    result.set("port", 3.14L); // long double: unsupported type, static_assert should fire
    return 0;
}
