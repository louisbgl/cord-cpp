# cord, a Config Reader

[![Tests](https://github.com/louisbgl/cord-cpp/actions/workflows/test.yml/badge.svg)](https://github.com/louisbgl/cord-cpp/actions)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Header-only](https://img.shields.io/badge/header--only-yes-brightgreen.svg)
![License](https://img.shields.io/badge/license-MIT-blue.svg)
![parse speed](https://img.shields.io/badge/parse%20speed-~42%C2%B5s-green.svg)

Header-only C++20 configuration parser with schema validation and fluent API.

## Requirements

- C++20 or later
- POSIX-compatible system (Linux, macOS). Windows is not supported.

## Features

- **Header-only**: zero dependencies
- **Type-safe**: compile-time checks via `static_assert`
- **Fluent API**: chain `.required()`, `.default_()`, `.min()`, `.max()`, `.oneOf()`
- **Error accumulation**: collect and inspect all errors at once
- **Strict/lenient modes**: reject or ignore unknown keys and duplicate keys
- **Write-back**: modify parsed values and serialize back to string or file

## Config File Format

- **Format:** `key = value` (one per line)
- **Whitespace:** Trimmed around keys and values
- **Strings:** Must be quoted with `""`. Supported escapes: `\"`, `\\`, `\n`, `\t`
- **Booleans:** `true` or `false`
- **Numbers:** `123` (int), `3.14` (float/double)
- **Vectors:** `[item1, item2, item3]` with square brackets
- **Comments:** `#` line or inline comments (when enabled)
- **Duplicate keys:** Last value wins (strict mode rejects duplicates)

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
cmake --build build --target run_constraints
cmake --build build --target run_writing
```

See the [`examples/`](examples/) directory for source:
- **[simplest](examples/simplest/)**: Primitives, required fields, defaults
- **[arrays](examples/arrays/)**: Vector support with `[]` syntax
- **[config_markers](examples/config_markers/)**: Custom delimiters and comment markers
- **[optionals](examples/optionals/)**: Safe optional field retrieval with `contains()` and `get_or()`
- **[constraints](examples/constraints/)**: Numeric range with `min()`/`max()` and choice validation with `oneOf()`
- **[writing](examples/writing/)**: Modify parsed values with `set()` and write back to disk

## Building & Testing

Requires CMake 3.15+ and a C++20 compiler.

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Benchmarks

All benchmarks measured on coldstart (single parse per run).   
Each benchmark binary supports `-h` for full options, thanks to [clap](https://github.com/lucaspujol/clap).

### Speed

Averaged over 10,000 runs:

- **Small config (10 keys)**: ~42 µs per parse
- **Large config (100 keys)**: ~290 µs per parse

Run yourself:

```bash
cmake -S . -B build
cmake --build build --target bench_speed
./build/benchmarks/bench_speed
```

### Memory

Static stack sizes:

- **Schema**: 128 bytes
- **Result**: 136 bytes
- **Value**: 48 bytes

Heap allocations scale with schema size and parsed data (unique_ptr per field, string/vector storage).

Run yourself:

```bash
cmake -S . -B build
cmake --build build --target bench_memory
./build/benchmarks/bench_memory
```

### vs Handcrafted Parser

TODO

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

// Numeric range constraints (int, float, double)
schema.add<int>("port").min(1024).max(65535);
schema.add<float>("ratio").min(0.0f).max(1.0f);

// String length constraints
schema.add<std::string>("username").min(3).max(32);

// Vector element count constraints
schema.add<std::vector<std::string>>("tags").min(1).max(10);

// Restrict to a set of allowed values
schema.add<std::string>("level").oneOf({"debug", "info", "warn", "error"});
schema.add<int>("verbosity").oneOf({0, 1, 2, 3});

// Comments (enabled by default)
schema.setAllowComments(true);

// Strict mode: reject unknown keys and duplicate keys (disabled by default)
schema.setStrict(true);

// Case-insensitive key matching: "PORT" matches field "port" (disabled by default)
schema.setCaseInsensitive(true);

// Custom delimiter (default: "=")
schema.setDelimiter(':');    // single char
schema.setDelimiter("==");  // or multi-char string

// Custom comment marker (default: "#")
schema.setCommentMarker(';');   // single char
schema.setCommentMarker("//"); // or multi-char string

// Print schema fields with types, modifiers, and constraints
// Groups by type family when field count exceeds 10
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
// Check if a key is present (parsed value or default)
bool exists = result.contains("port");

// Get and convert, throws CordException if key missing or type wrong
int port         = result.get("port").as<int>();
std::string host = result.get("host").as<std::string>();

// Safe fallback, fallback can be a runtime value
int port = result.get_or("port", env == "prod" ? 443 : 8080).as<int>();
```

### Write-back

```cpp
// Modify a parsed value (or add a new key)
result.set("port", 9090)
      .set("host", std::string("example.com"));

// Serialize to string (uses same delimiter as the schema)
std::string config = result.write();

// Write to file
result.writeFile("output.conf");

// Full round-trip: load, modify, save
auto result = schema.parseFile("config.conf");
result.set("high_score", 9999).writeFile("config.conf");
```

## License

See LICENSE file.
