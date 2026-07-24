// This file must NOT compile — verifies unsupported types are rejected at compile time
#include "cord.hpp"

int main() {
    cord::Schema schema;
    schema.add<char>("invalid_type"); // static_assert should fire here
    return 0;
}
