#include "cord.hpp"

#include <iostream>

// I recommend peeking at examples/simplest/simplest.cpp first :)

// cord supports value constraints on schema fields, checked at parse time
// Violations accumulate as errors in the result, same as type errors or missing required fields

int main() {
    cord::Schema schema;

    // min() and max() enforce numeric range, available on int, float, double
    // port must be a valid unprivileged port number
    schema.add<int>("port").required().min(1024).max(65535);

    // workers defaults to 1, but must stay in a sane range
    schema.add<int>("workers").default_(1).min(1).max(64);

    // ratio must be in [0.0, 1.0]
    schema.add<float>("ratio").default_(0.5f).min(0.0f).max(1.0f);

    // oneOf() restricts a field to a fixed set of allowed values, works on any supported type
    schema.add<std::string>("log_level").default_("info").oneOf({"debug", "info", "warn", "error"});

    // min()/max() and oneOf() can be combined on numeric fields
    // Here timeout_ms must be in [100, 30000] AND one of the accepted preset values
    schema.add<int>("timeout_ms").default_(1000).min(100).max(30000).oneOf({100, 500, 1000, 5000, 30000});

    auto result = schema.parseFile("constraints.conf");

    if (result.hasErrors()) {
        result.printErrors();
        return 1;
    }

    std::cout << "Parsed values:" << std::endl;
    std::cout << "port:       " << result.get("port").as<int>() << std::endl;
    std::cout << "workers:    " << result.get("workers").as<int>() << std::endl;
    std::cout << "ratio:      " << result.get("ratio").as<float>() << std::endl;
    std::cout << "log_level:  " << result.get("log_level").as<std::string>() << std::endl;
    std::cout << "timeout_ms: " << result.get("timeout_ms").as<int>() << std::endl;

    return 0;
}
