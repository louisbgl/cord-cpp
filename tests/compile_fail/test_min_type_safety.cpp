// This file must NOT compile — verifies min() rejects non-numeric types at compile time
#include "cord.hpp"

int main() {
    cord::Schema schema;
    schema.add<std::string>("host").min("a"); // static_assert should fire here
    return 0;
}
