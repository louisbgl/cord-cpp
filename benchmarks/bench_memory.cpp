#include "clap.hpp"
#include "cord.hpp"

#include <iostream>

int main(int argc, char** argv) {
    clap::App app(argv[0], "Benchmark for cord memory footprint");

    auto& help = app.flag("-h,--help", "Show this help message");
    auto& raw = app.flag("--raw", "Output raw bytes only (schema result value)");

    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help() << std::endl; return 0; }
    if (!ok)  { std::cerr << app.help() << std::endl; return 1; }

    bool raw_mode = raw;

    size_t schema_size = sizeof(cord::Schema);
    size_t result_size = sizeof(cord::Result);
    size_t value_size  = sizeof(cord::Value);

    if (raw_mode) {
        std::cout << schema_size << "\n";
        std::cout << result_size << "\n";
        std::cout << value_size << "\n";
    } else {
        std::cout << "Static sizeof:\n";
        std::cout << "  Schema: " << schema_size << " bytes\n";
        std::cout << "  Result: " << result_size << " bytes\n";
        std::cout << "  Value:  " << value_size << " bytes\n";
        std::cout << "\n";
        std::cout << "Heap allocations:\n";
        std::cout << "  - unique_ptr<IField> per schema field\n";
        std::cout << "  - std::string copies for keys and string values\n";
        std::cout << "  - std::vector storage for array values\n";
        std::cout << "  - Result stores vector<pair<string, Value>> (not a map)\n";
        std::cout << "\n";
        std::cout << "Profile heap usage with valgrind:\n";
        std::cout << "  valgrind --tool=massif ./build/benchmarks/bench_memory\n";
        std::cout << "  ms_print massif.out.<pid>\n";
    }

    return 0;
}
