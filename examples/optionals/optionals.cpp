#include "../../cord.hpp"

#include <iostream>

int main() {
    cord::Schema schema;
    schema.add<int>("port");
    schema.add<std::string>("env").required();

    auto result = schema.parseFile("examples/optionals/optionals.conf");

    if (result.hasErrors()) result.printErrors();
    else {
        std::cout << "Parsed values:" << std::endl;

        std::string env = result.get("env").as<std::string>();

        // Say port depends on env being prod or something else
        // int port = result.get("port").as<int>();
        // ^ will throw if port is not in the config file
        // Instead, use get_or() to provide a potentially dynamic fallback value
        // Useful if a config value isnt required but depends on another value in the config file
        int port = result.get_or("port", env == "prod" ? 443 : 8080).as<int>();

        std::cout << "port: " << port << std::endl;
        std::cout << "env: "  << env  << std::endl;
        }

    return 0;
}