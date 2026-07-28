#include "../src/cord.hpp"

#include <iostream>

int main() {
    cord::Schema schema;

    schema.add<int>("port").required();
    schema.add<std::string>("host").default_("localhost");
    schema.add<bool>("debug").default_(false);

    schema.describe();
    std::cout << std::endl;

    auto result = schema.parse(R"(
        port=8080
        host="example.com"
        debug=true
    )");

    if (result.hasErrors()) {
        result.printErrors();
        return 1;
    }
    
    std::cout << "Parsed values:" << std::endl;

    std::cout << "port: " << result.get("port").as<int>()                         << std::endl;
    std::cout << "host: " << result.get("host").as<std::string>()                 << std::endl;
    std::cout << "debug: " << (result.get("debug").as<bool>() ? "true" : "false") << std::endl;
    std::cout << std::endl;

    std::cout << "After modifications:" << std::endl;
    result.set("port", 6767).set("debug", false);

    std::cout << "port: " << result.get("port").as<int>()                         << std::endl;
    std::cout << "host: " << result.get("host").as<std::string>()                 << std::endl;
    std::cout << "debug: " << (result.get("debug").as<bool>() ? "true" : "false") << std::endl;

    return 0;
}