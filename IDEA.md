# cord — Config Reader

## Overview
Header-only C++ configuration parsing library with schema validation and fluent API.

## Distribution
- **Header-only**, namespaced under `cord::`
- Structure:
  - `include/cord/schema.hpp`
  - `include/cord/field.hpp`
  - `include/cord/errors.hpp`
  - `include/cord.hpp` (umbrella header)

## Config File Format
- Flat `key = value`, one declaration per line
- Whitespace ignored except inside strings
- Strings always quoted
- Supported types: `int`, `float`, `bool` (true/false only), `std::string`
- Comments: `#` to end of line, toggled via `schema.enableComments(bool)`
- **No arrays, no sections/nesting, no interpolation** (explicitly deferred)
- Dotted-key naming reserved internally for future nesting support

## Schema Definition
- Fluent API: `schema.add<int>("port").required();` / `.default_(value)`
- `required()` + `default_()` misuse caught via runtime assert, documented clearly
- Field types modeled via `IField` interface + `Field<T>` implementations
  - Chosen over variant to leave door open for custom field types later
- Fields stored as `vector<unique_ptr<IField>>` for stable addresses
  - Needed since `add<T>()` returns live reference for chaining

## Validation Policy
- `schema.strict(bool)` — unknown key in file: error vs ignore
- Duplicate keys: **last wins**, no toggle
- All errors collected and reported together (not fail-fast)

## Parsing / Input
- Core parse takes `std::string_view`
- Thin wrapper loads from file path

## Result Access
- `schema.parse(...)` returns a `Result`
- `result.get("key")` → returns a `Value` (or reports missing key)
- `value.as<int>()` → explicit templated conversion (or reports type mismatch)
- Chainable: `result.get("port").as<int>()`
- `Value` internally wraps `variant<int, float, bool, string, ...>`
  - Designed so a future `Result` alternative slots into the same variant for nesting, without changing this API

## Debug Tooling
- `schema.describe()` — prints boolean config state + field list as pseudo-POD struct text
- Debug-only, not codegen

## Explicitly Deferred
- Nested sections
- Arrays/lists
- Interpolation
- Configurable comment chars
- Configurable duplicate-key behavior
- Compile-time type-state validation
- Codegen from `describe()`
- Custom-type variant expansion beyond the 4 scalars
