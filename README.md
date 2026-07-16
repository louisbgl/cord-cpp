# cord, a Config Reader

Header-only C++ configuration parser with schema validation and fluent API.

## Requirements

C++20 or later. That's it.

## Features

- **Header-only**: zero dependencies
- **Type-safe**: compile-time checks via `static_assert`
- **Fluent API**: chain `.required()`, `.default_()`
- **Error accumulation**: report all errors at once
- **Strict/lenient modes**: reject or ignore unknown keys
- **Comment support**: optional `#` line comments

## Config File Format

- **Format:** `key = value` (one per line)
- **Whitespace:** Trimmed around keys and values
- **Strings:** Must be quoted with `""`
- **Booleans:** `true` or `false`
- **Numbers:** `123` (int), `3.14` (float/double)
- **Vectors:** `[item1, item2, item3]` with square brackets
- **Comments:** `#` to end of line (when enabled)
- **Duplicate keys:** Last value wins

## Installation

cord ships as a single header. You only need `cord.hpp`.

### Option 1: Download the header

Download the header into your project:

```bash
curl -O https://raw.githubusercontent.com/louisbgl/cord-cpp/main/cord.hpp
```

Then include it in your code:

```cpp
#include "cord.hpp"
```

### Option 2: CMake FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(cord
  GIT_REPOSITORY https://github.com/louisbgl/cord-cpp.git
  GIT_TAG main)
FetchContent_MakeAvailable(cord)

target_link_libraries(your_app PRIVATE cord)
```

Then include in your code:

```cpp
#include "cord.hpp"
```

## Quick Start

```cpp
#include "cord.hpp"

int main() {
    // Define schema
    cord::Schema schema;
    schema.add<int>("port").required();
    schema.add<std::string>("host").default_("localhost");
    schema.add<bool>("debug").default_(false);
    schema.add<double>("timeout").default_(30.0);

    // Parse config file
    auto result = schema.parseFile("config.txt");

    // Check for errors
    if (result.hasErrors()) {
        result.printErrors();
        return 1;
    }

    // Access values
    int port = result.get("port").as<int>();
    std::string host = result.get("host").as<std::string>();
    bool debug = result.get("debug").as<bool>();

    return 0;
}
```

**Example config file:**
```ini
port = 8080
host = "example.com"
debug = true
timeout = 45.5
```

## API Reference

### Schema Definition

```cpp
cord::Schema schema;

// Add fields with type
schema.add<int>("name");
schema.add<float>("name");
schema.add<double>("name");
schema.add<bool>("name");

// Also in vector form
schema.add<std::string>("name");
schema.add<std::vector<int>>("name");
schema.add<std::vector<float>>("name");
schema.add<std::vector<double>>("name");
schema.add<std::vector<bool>>("name");
schema.add<std::vector<std::string>>("name");

// Mark as required
schema.add<int>("port").required();

// Set default value
schema.add<std::string>("host").default_("localhost");

// Enable comments in config file
schema.setAllowComments(true);

// Strict mode (reject unknown keys)
schema.setStrict(true);

// Debug: print schema structure
schema.describe();
```

### Parsing

```cpp
// Parse from string
auto result = schema.parse(config_string);

// Parse from file
auto result = schema.parseFile("config.txt");
```

### Error Handling

```cpp
// Check for errors
if (result.hasErrors()) {
    result.printErrors();  // Print to stderr
}
```

### Value Access

```cpp
// Get and convert value
int port = result.get("port").as<int>();
std::string host = result.get("host").as<std::string>();

// Throws CordException if:
// - Key not found
// - Type mismatch in as<T>()
```

## License

See LICENSE file.
