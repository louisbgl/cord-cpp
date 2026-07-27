#include "cord.hpp"

#include <iostream>

int main() {
    cord::Schema schema;
    schema.add<int>("port");
    schema.add<std::string>("env").required();

    schema.describe();
    std::cout << std::endl;

    auto result = schema.parseFile("optionals.conf");

    if (result.hasErrors()) {
        result.printErrors();
        return 1;
    }

    std::cout << "Parsed values:" << std::endl;

    std::string env = result.get("env").as<std::string>();

    // contains() checks if a key is present in the result (parsed or default)
    // Useful when presence itself carries meaning
    if (result.contains("port")) {
        std::cout << "port: " << result.get("port").as<int>() << std::endl;
    } else {
        // get_or() for a runtime fallback when the key is absent
        int port = result.get_or("port", env == "prod" ? 443 : 8080).as<int>();
        std::cout << "port: " << port << " (default for env=" << env << ")" << std::endl;
    }

    std::cout << "env: " << env << std::endl;

    return 0;
}
