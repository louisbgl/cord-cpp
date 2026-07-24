# cord, a Config Reader

Header-only C++20 configuration parser with schema validation and fluent API.

## Requirements

- C++20 or later
- POSIX-compatible system (Linux, macOS). Windows is not supported.

## Features

- **Header-only**: zero dependencies
- **Type-safe**: compile-time checks via `static_assert`
- **Fluent API**: chain `.required()`, `.default_()`
- **Error accumulation**: collect and inspect all errors at once
- **Strict/lenient modes**: reject or ignore unknown keys

## Config File Format

- **Format:** `key = value` (one per line)
- **Whitespace:** Trimmed around keys and values
- **Strings:** Must be quoted with `""`
- **Booleans:** `true` or `false`
- **Numbers:** `123` (int), `3.14` (float/double)
- **Vectors:** `[item1, item2, item3]` with square brackets
- **Comments:** `#` line or inline comments (when enabled)
- **Duplicate keys:** Last value wins

## Installation

cord ships as a single header. You only need `cord.hpp`.

### Option 1: Download the header

```bash
curl -O https://raw.githubusercontent.com/louisbgl/cord-cpp/main/cord.hpp
```

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

```cpp
#include "cord.hpp"
```

## Examples

Build and run examples with CMake from the project root:

```bash
cmake -S . -B build && cmake --build build

cmake --build build --target run_simplest
cmake --build build --target run_arrays
cmake --build build --target run_optionals
cmake --build build --target run_config_markers
```

See the [`examples/`](examples/) directory for source:
- **[simplest](examples/simplest/)**: Primitives, required fields, defaults
- **[arrays](examples/arrays/)**: Vector support with `[]` syntax
- **[config_markers](examples/config_markers/)**: Custom delimiters and comment markers
- **[optionals](examples/optionals/)**: Safe optional field retrieval with `get_or()`

## Building & Testing

Requires CMake 3.15+ and a C++20 compiler.

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## API Reference

### Schema Definition

```cpp
cord::Schema schema;

// Scalar types
schema.add<int>("name");
schema.add<float>("name");
schema.add<double>("name");
schema.add<bool>("name");
schema.add<std::string>("name");

// Vector types
schema.add<std::vector<int>>("name");
schema.add<std::vector<float>>("name");
schema.add<std::vector<double>>("name");
schema.add<std::vector<bool>>("name");
schema.add<std::vector<std::string>>("name");

// Mark as required (throws CordException if missing at parse time)
schema.add<int>("port").required();

// Set default value (used when key is absent)
schema.add<std::string>("host").default_("localhost");

// Comments (enabled by default)
schema.setAllowComments(true);

// Strict mode: reject unknown keys (disabled by default)
schema.setStrict(true);

// Custom delimiter (default: "=")
schema.setDelimiter(':');    // single char
schema.setDelimiter("==");  // or multi-char string

// Custom comment marker (default: "#")
schema.setCommentMarker(';');   // single char
schema.setCommentMarker("//"); // or multi-char string

// Print schema as a C-style struct (useful for debugging)
schema.describe();
```

### Parsing

```cpp
auto result = schema.parse(config_string);     // from string
auto result = schema.parseFile("config.txt");  // from file
```

### Error Handling

```cpp
if (result.hasErrors()) {
    result.printErrors();  // print all errors to stderr

    // or inspect programmatically
    for (const auto& err : result.getErrors()) {
        std::cerr << err.message;
        if (err.line.has_value()) std::cerr << " (line " << *err.line << ")";
        std::cerr << "\n";
    }
}
```

### Value Access

```cpp
// Get and convert, throws CordException if key missing or type wrong
int port         = result.get("port").as<int>();
std::string host = result.get("host").as<std::string>();

// Safe fallback, fallback can be a runtime value
int port = result.get_or("port", env == "prod" ? 443 : 8080).as<int>();
```

## License

See LICENSE file.
