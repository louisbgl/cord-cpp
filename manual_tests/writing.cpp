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

    std::cout << result.write() << std::endl << std::endl;

    std::cout << "After modifications:" << std::endl;
    result.set("port", 6767).set("debug", false);

    std::cout << result.write() << std::endl;

    return 0;
}