#include "../../cord.hpp"

#include <iostream>

// I recommend peeking at examples/simplest/simplest.cpp first :)

// I'll note here that we only support arrays of primitives, meaning no arrays of arrays
// This is type checked at compile time
// All user-facing methods that are templated do compile time checks on the types passed in for safety

int main() {
    cord::Schema schema;
    schema.setAllowComments(true);

    schema.add<std::string>("name").required();
    schema.add<int>("level").default_(1);

    // Here we say the default for the "items" field is an empty vector of strings
    schema.add<std::vector<std::string>>("items").default_({});

    schema.describe();
    std::cout << std::endl;

    auto result = schema.parseFile("examples/arrays/arrays.conf");
    if (result.hasErrors()) result.printErrors();
    else {
        std::cout << "Parsed values:" << std::endl;
        std::cout << "name: " << result.get("name").as<std::string>() << std::endl;
        std::cout << "level: " << result.get("level").as<int>() << std::endl;
        std::cout << "items: ";
        for (const auto& item : result.get("items").as<std::vector<std::string>>()) {
            std::cout << item << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}