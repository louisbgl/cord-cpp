// This file must NOT compile — verifies max() rejects non-numeric types at compile time
#include "cord.hpp"

int main() {
    cord::Schema schema;
    schema.add<std::string>("host").max("z"); // static_assert should fire here
    return 0;
}
