#include "../src/cord.hpp"

#include <iostream>

int main() {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.add<std::string>("name").required();
    schema.add<int>("level").default_(1);
    schema.add<std::vector<std::string>>("items").default_({});

    schema.describe();
    std::cout << std::endl;

    auto result = schema.parseFile("manual_tests/test2.conf");
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