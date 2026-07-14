# cord, a Config Reader

Header-only C++ configuration parser with schema validation and fluent API.

## Features

- **Header-only**, zero dependencies, C++20
- **Type-safe** schema definition with compile-time checks
- **Fluent API** for clean, readable schema definitions
- **Error accumulation** — reports all errors together, not fail-fast
- **Strict/lenient** parsing modes for unknown keys
- **Comment support** — optional `#` line comments

## Config File Format

- **Format:** `key = value` (one per line)
- **Whitespace:** Trimmed around keys and values
- **Strings:** Must be quoted with `""`
- **Booleans:** `true` or `false` only
- **Numbers:** `123` (int) or `3.14` (double)
- **Comments:** `#` to end of line (when enabled)
- **Duplicate keys:** Last value wins

## Building

Header-only library. Just include `cord.hpp` in your file!   
You have to copy all hpp file from within `src` to your project tho :)   

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
schema.add<double>("name");
schema.add<bool>("name");
schema.add<std::string>("name");

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

## Running Tests

```bash
./run_tests.sh
```

## License

See LICENSE file.
