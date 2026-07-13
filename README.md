# cord

Header-only C++ config parser with schema validation and fluent API.

## Quick Start

```cpp
#include <cord.hpp>

cord::Schema schema;
schema.add<int>("port").required();
schema.add<std::string>("host").default_("localhost");
schema.add<bool>("debug").default_(false);

auto result = schema.parse(configText);
int port = result.get("port").as<int>();
```

## Features

- Header-only, no dependencies
- Fluent schema definition API
- Type-safe value access
- Batch error reporting
- Strict/lenient mode for unknown keys

See **IDEA.md** for complete design doc.
