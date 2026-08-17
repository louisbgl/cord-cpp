#include "clap.hpp" // Ty kokonut for clap :heart:
#include "stu.hpp" // i made this lib too
#include "cord.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>

// Minimal hand-rolled parser: find + substr, no validation, no type conversion
// Assumes you know the exact keys upfront
static std::unordered_map<std::string, std::string> handcrafted_parse(std::string_view config) {
    std::unordered_map<std::string, std::string> result;

    auto extract = [&](const char* key) {
        size_t pos = config.find(key);
        if (pos == std::string_view::npos) return;

        pos += std::strlen(key);
        size_t eq = config.find('=', pos);
        if (eq == std::string_view::npos) return;

        size_t val_start = eq + 1;
        while (val_start < config.size() && std::isspace(config[val_start])) ++val_start;

        size_t val_end = config.find('\n', val_start);
        if (val_end == std::string_view::npos) val_end = config.size();

        std::string value(config.substr(val_start, val_end - val_start));
        // Strip quotes if present
        if (!value.empty() && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }

        result[key] = value;
    };

    extract("port");
    extract("host");
    extract("timeout");
    extract("max_connections");
    extract("log_level");
    extract("buffer_size");
    extract("retry_count");
    extract("db_host");
    extract("api_key");
    extract("workers");

    return result;
}

static std::string_view test_config = R"(
port = 8080
host = "localhost"
timeout = 30
max_connections = 100
log_level = "info"
buffer_size = 4096
retry_count = 3
db_host = "db.example.com"
api_key = "secret123"
workers = 8
)";

int main(int argc, char** argv) {
    clap::App app(argv[0], "Benchmark cord vs hand-rolled parser");

    auto& help = app.flag("-h,--help", "Show this help message");
    auto& iter = app.option<int>("-i,--iterations", "Number of iterations to run").default_value(1);
    auto& raw = app.flag("--raw", "Output raw overhead nanoseconds only");

    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help() << std::endl; return 0; }
    if (!ok)  { std::cerr << app.help() << std::endl; return 1; }

    int iterations = iter.get();
    bool raw_mode = raw;

    // --- Cord parser ---
    cord::Schema schema;
    schema.add<int>("port");
    schema.add<std::string>("host");
    schema.add<int>("timeout");
    schema.add<int>("max_connections");
    schema.add<std::string>("log_level");
    schema.add<int>("buffer_size");
    schema.add<int>("retry_count");
    schema.add<std::string>("db_host");
    schema.add<std::string>("api_key");
    schema.add<int>("workers");

    stu::Instant t0 = stu::Instant::now();
    for (int i = 0; i < iterations; ++i) {
        schema.parse(test_config);
    }
    stu::Instant t1 = stu::Instant::now();
    stu::Duration cord_time = t1 - t0;

    // --- Hand-rolled parser ---
    stu::Instant t2 = stu::Instant::now();
    for (int i = 0; i < iterations; ++i) {
        handcrafted_parse(test_config);
    }
    stu::Instant t3 = stu::Instant::now();
    stu::Duration handcrafted_time = t3 - t2;

    stu::Duration overhead = cord_time - handcrafted_time;

    if (raw_mode) {
        std::cout << overhead.in_ns() / iterations << "\n";
    } else {
        std::cout << "10-key config, " << iterations << " iteration" << (iterations == 1 ? "" : "s") << "\n";
        std::cout << "\n";
        std::cout << "  cord: " << cord_time / iterations << "\n";
        std::cout << "    - Schema validation\n";
        std::cout << "    - Type conversion (int, string, vectors, etc.)\n";
        std::cout << "    - Error accumulation & reporting\n";
        std::cout << "    - Constraints (min/max/oneOf)\n";
        std::cout << "    - Escape sequence handling\n";
        std::cout << "\n";
        std::cout << "  hand-rolled: " << handcrafted_time / iterations << "\n";
        std::cout << "    - Raw find+substr, strings only\n";
        std::cout << "    - No validation, no errors\n";
        std::cout << "    - No types (bool/int/vectors)\n";
        std::cout << "    - No escapes, comments, or edge cases\n";
        std::cout << "\n";
        std::cout << "  overhead:    " << overhead / iterations << "\n";
    }

    return 0;
}
