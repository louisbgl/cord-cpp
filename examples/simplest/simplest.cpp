#include "../../cord.hpp"

#include <iostream>

int main() {
    // A Schema is the representation of expected values in a config file
    cord::Schema schema;

    schema.setAllowComments(true); // not necessary, defaults to false
    schema.setStrict(false); // not necessary, defaults to false

    // Note that when you add a field, it's not necesary to have it be required or have a default value
    // A field can not be both required and have a default, that's weird
    // A field can be neither required nor have a default, in which case using result.get()
    // on that field will throw an exception if the field is not present in the config file
    schema.add<int>("port").required();
    schema.add<std::string>("host").default_("localhost");
    schema.add<bool>("debug").default_(false);

    // Describe here is mainly to visualize the schema as a c-style struct
    // Useful for debugging or visualizing the schema
    schema.describe();
    std::cout << std::endl;

    // Could also use schema.parse() on a string if wanted
    // parseFile is just a wrapper around schema.parse()
    auto result = schema.parseFile("examples/simplest/simplest.conf");

    // Result is a container for the parsed values and any errors that may have occurred during parsing
    if (result.hasErrors()) result.printErrors();
    else {
        std::cout << "Parsed values:" << std::endl;

        // When getting values from the result, it's advised to chain result.get() with .as<T>() to get the value as the expected type
        // result.get() can throw if the key is not present in the result
        // .as<T>() can throw if the value is not of the expected type
        // .as<T>() is nice because it makes the type gotten clear and safe
        std::cout << "port: " << result.get("port").as<int>()                         << std::endl;
        std::cout << "host: " << result.get("host").as<std::string>()                 << std::endl;
        std::cout << "debug: " << (result.get("debug").as<bool>() ? "true" : "false") << std::endl;
    }

    return 0;
}