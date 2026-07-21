#include "../../cord.hpp"

#include <iostream>

int main() {
    // A Schema is the representation of expected values in a config file
    cord::Schema schema;

    schema.setAllowComments(true); // not necessary, defaults to true anyways
    schema.setStrict(false); // not necessary, defaults to false anyways

    schema.add<int>("port").required();
    schema.add<std::string>("host").default_("localhost");
    schema.add<bool>("debug").default_(false);

    // Describe here is mainly to visualize the schema as a c-style struct
    // useful for debugging or visualizing the schema
    schema.describe();
    std::cout << std::endl;

    // Could also use schema.parse() on a string if wanted
    // parseFile is just a wrapper around schema.parse()
    auto result = schema.parseFile("examples/simplest/simplest.conf");

    // Result is a container for the parsed values and any errors that may have occurred during parsing
    if (result.hasErrors()) result.printErrors();
    else {
        std::cout << "Parsed values:" << std::endl;
        std::cout << "port: " << result.get("port").as<int>()                         << std::endl;
        std::cout << "host: " << result.get("host").as<std::string>()                 << std::endl;
        std::cout << "debug: " << (result.get("debug").as<bool>() ? "true" : "false") << std::endl;
    }

    return 0;
}