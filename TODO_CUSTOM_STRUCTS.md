# Custom Struct Support - Remaining Work

## Implemented ✓
- Basic single custom struct parsing
- Vector of custom structs parsing
- Field constraints (required, default, min/max, oneOf) on primitive fields within custom structs
- Type safety via `CORD_REGISTER_STRUCT` macro
- Compile-time checks for unregistered types
- Comprehensive test coverage (16 tests)

## Not Yet Implemented

### 1. `describe()` for Custom Structs
Currently shows generic type name ("Artist", "Track") but not field details.

**Should show:**
```
Artist {
  string  name         (required)
  int     year_formed  [1900..2024]
  string  genre        (default="Unknown")
}
```

**Location:** `Schema::describe()` in `schema.hpp`

### 2. Constraints on Vector<CustomStruct> Fields
Constraints like `min()/max()` on the vector field itself (element count) don't compile.

**Current:** `schema.add<vector<Track>>("tracks", track_def).min(1);` → static_assert fails

**Reason:** `min(size_t)` overload checks `is_supported_vector_type_v<T>` which excludes custom types.

**Fix:** Add support for vector element count constraints on custom struct vectors, or document limitation.

**Location:** `Field<T>::min(size_t)` / `max(size_t)` in `field.hpp` lines 510, 540

### 3. Nested Custom Structs
Can't have custom struct fields inside other custom structs.

**Example (doesn't work):**
```cpp
struct Address {
    std::string city = "";
    int zip = 0;
};
CORD_REGISTER_STRUCT(Address);

struct Person {
    std::string name = "";
    Address address;  // <- nested custom struct
};
CORD_REGISTER_STRUCT(Person);
```

**Blocker:** `CustomStruct::add()` line 25 only accepts primitives (`is_supported_value_type_v<FieldType>`).

**Fix needed:**
- Add `CustomStruct::add()` overload accepting `CustomField<NestedType>`
- Update `valueToString()` to handle nested custom types
- Recursive parsing in `_tryParseCustomObject()`

**Files:** `custom_struct.hpp`, `common.hpp::valueToString()`, `schema.hpp::_tryParseCustomObject()`

### 4. Multiline Config Values
Current limitation: all values must fit on single line. Affects readability of large arrays/objects.

**Example (doesn't parse):**
```
tracks = [
    { title = "Song 1", duration = 180 },
    { title = "Song 2", duration = 240 }
]
```

**Reason:** Line-oriented parser splits on `\n` first, can't track bracket depth across lines.

**Fix:** Architectural — see issue #35 (lexer/parser refactor)

## Nice to Have

- Vector element count constraints on custom struct vectors
- `describe()` formatting options (compact vs expanded for custom structs)
- Source maps for custom struct field errors (show exact position in config)
- Custom validators on struct fields

---

# CI Improvements

## Current State
- Ubuntu only (default GCC)
- Single compiler, single platform
- Examples workflow missing `run_custom_types`

## Needed

### 1. Compiler Matrix (Linux)
**Priority: High**

Test multiple compilers to catch platform-specific issues:
- GCC: 11, 12, 13
- Clang: 15, 16, 17

Add matrix strategy to `.github/workflows/test.yml`:
```yaml
strategy:
  matrix:
    compiler:
      - { cc: gcc-11, cxx: g++-11 }
      - { cc: gcc-12, cxx: g++-12 }
      - { cc: gcc-13, cxx: g++-13 }
      - { cc: clang-15, cxx: clang++-15 }
      - { cc: clang-16, cxx: clang++-16 }
      - { cc: clang-17, cxx: clang++-17 }
```

### 2. macOS Support
**Priority: Medium**

Add macOS job with AppleClang:
- `runs-on: macos-latest`
- Tests M1/M2 compatibility
- Different standard library implementation

### 3. Windows MSVC
**Priority: Low (Defer)**

Add Windows job:
- `runs-on: windows-latest`
- MSVC C++20 support varies by version
- May need code adjustments for compatibility
- Can be disabled initially until Windows compatibility verified

**Note:** Mark job as `continue-on-error: true` until full Windows support confirmed.
