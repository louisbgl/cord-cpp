#include "clap.hpp" // Ty kokonut for clap :heart:
#include "stu.hpp" // i made this too hihi
#include "cord.hpp"

#include <iostream>
#include <string>
#include <string_view>

static void print_results(std::string_view label, stu::Duration time,
                          std::string_view input, int iterations) {
    double input_mb    = (static_cast<double>(input.size()) * iterations) / (1024.0 * 1024.0);
    double throughput  = input_mb / (time.in_ns() / 1e9);

    std::cout << label << "\n";
    std::cout << "\ttotal:      " << time << std::endl;
    std::cout << "\tper parse:  " << time / iterations << std::endl;
    std::cout << "\tthroughput: " << throughput << " MB/s\n";
}

std::string_view small_config = R"(
key1 = 123
key2 = "hello world"
key3 = [1, 2, 3, 4, 5]
key4 = true
key5 = 3.14
key6 = [1.1, 2.2, 3.3]
key7 = ["a", "b", "c"]
key8 = [true, false, true]
key9 = "Yeah so i have no idea what to make key9 be so this is just going to be very long sorry."
key10 = 67
)";

// cycles through all 10 supported cord types, keyed by index % 10
static void add_key(cord::Schema& schema, const std::string& name, int i) {
    switch (i % 10) {
        case 0: schema.add<int>(name); break;
        case 1: schema.add<float>(name); break;
        case 2: schema.add<double>(name); break;
        case 3: schema.add<bool>(name); break;
        case 4: schema.add<std::string>(name); break;
        case 5: schema.add<std::vector<int>>(name); break;
        case 6: schema.add<std::vector<float>>(name); break;
        case 7: schema.add<std::vector<double>>(name); break;
        case 8: schema.add<std::vector<bool>>(name); break;
        case 9: schema.add<std::vector<std::string>>(name); break;
    }
}

static std::string value_for(int i) {
    switch (i % 10) {
        case 0: return "42";
        case 1: return "1.5";
        case 2: return "3.14";
        case 3: return "true";
        case 4: return "\"hello\"";
        case 5: return "[1, 2, 3]";
        case 6: return "[1.1, 2.2]";
        case 7: return "[3.14, 2.71]";
        case 8: return "[true, false]";
        case 9: return "[\"a\", \"b\"]";
        default: return "0";
    }
}

int main(int argc, char** argv) {
    // CLAP setup
    clap::App app(argv[0], "Benchmark for cord speed");

    auto& help = app.flag("-h,--help", "Show this help message");
    auto& iter = app.option<int>("-i,--iterations", "Number of iterations to run").default_value(1);
    auto& raw = app.flag("--raw", "Output raw nanoseconds only (small_ns large_ns)");

    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help() << std::endl; return 0; }
    if (!ok)  { std::cerr << app.help() << std::endl; return 1; }

    int iterations = iter.get();
    bool raw_mode = raw;

    // --- Small config (10 keys, hand-written, all types) ---
    cord::Schema schema_small;

    schema_small.add<int>("key1");
    schema_small.add<std::string>("key2");
    schema_small.add<std::vector<int>>("key3");
    schema_small.add<bool>("key4");
    schema_small.add<double>("key5");
    schema_small.add<std::vector<float>>("key6");
    schema_small.add<std::vector<std::string>>("key7");
    schema_small.add<std::vector<bool>>("key8");
    schema_small.add<std::string>("key9");
    schema_small.add<int>("key10");

    // Time the parse loop
    stu::Instant t0 = stu::Instant::now();
    for (int i = 0; i < iterations; ++i) {
        schema_small.parse(small_config); // we dont care about the parse result here
    }
    
    stu::Instant t1 = stu::Instant::now();
    stu::Duration small_time = t1 - t0;

    if (!raw_mode) {
        print_results("Small config (10 keys), " + std::to_string(iterations) + " iterations",
                      small_time, small_config, iterations);
    }

    // --- Large config (1000 keys, runtime generated, cycling all types) ---
    static constexpr int BIG_KEY_COUNT = 100;

    cord::Schema schema_large;
    std::string big_config;
    for (int i = 0; i < BIG_KEY_COUNT; ++i) {
        std::string name = "key" + std::to_string(i + 1);
        add_key(schema_large, name, i);
        big_config += name + " = " + value_for(i) + "\n";
    }

    stu::Instant t2 = stu::Instant::now();
    for (int i = 0; i < iterations; ++i) {
        schema_large.parse(big_config); // we dont care about the parse result here
    }
    stu::Instant t3 = stu::Instant::now();
    stu::Duration large_time = t3 - t2;

    if (!raw_mode) {
        std::cout << "\n";
        print_results("Large config (" + std::to_string(BIG_KEY_COUNT) + " keys), " + std::to_string(iterations) + " iterations",
                      large_time, big_config, iterations);
    } else {
        std::cout << small_time.in_ns() << "\n";
        std::cout << large_time.in_ns() << "\n";
    }

    return 0;
}