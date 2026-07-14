#pragma once

// This is simply an umbrella header

#include "schema.hpp"

namespace cord {

// Convenience, also makes linters happy about unused includes
inline Schema makeSchema() {
    return Schema();
}

// If you want to make linters happy about unused includes, you can call this macro in your main() function.
// Intentional no-op
#define CORD_MAKE_LINTER_HAPPY() ((void)0)

} // namespace cord