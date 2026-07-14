#pragma once

// This is simply an umbrella header

#include "schema.hpp"

namespace cord {

// Convenience, also makes linters happy about unused includes
inline Schema makeSchema() {
    return Schema();
}

} // namespace cord