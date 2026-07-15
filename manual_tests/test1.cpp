#include "../src/cord.hpp"

#include <iostream>

int main() {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.add<int>("port").required();
    schema.add<std::string>("host").default_("localhost");
    schema.add<bool>("debug").default_(false);

    schema.describe();
    std::cout << std::endl;

    auto result = schema.parseFile("manual_tests/test1.conf");
    if (result.hasErrors()) result.printErrors();
    else {
        std::cout << "Parsed values:" << std::endl;
        std::cout << "port: " << result.get("port").as<int>() << std::endl;
        std::cout << "host: " << result.get("host").as<std::string>() << std::endl;
        std::cout << "debug: " << (result.get("debug").as<bool>() ? "true" : "false") << std::endl;
    }

    return 0;
}