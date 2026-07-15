// Test that unsupported types produce clear compile errors
// This file should NOT compile successfully

#ifdef USE_SINGLE_HEADER
#include "cord.hpp"
#else
#include "../src/cord.hpp"
#endif

int main() {
    cord::Schema schema;

    // This should fail with clear error message about unsupported type
    schema.add<char>("invalid_type");

    return 0;
}
