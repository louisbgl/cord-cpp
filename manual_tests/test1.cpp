#include "../src/cord.hpp"

#include <iostream>

int main() {
    cord::Schema schema;
    schema.setAllowComments(false);
    schema.add<int>("port").required();
    schema.add<std::string>("host").default_("localhost");
    schema.add<bool>("debug").default_(false);

    schema.describe();

    std::string input = R"(port=8080
debug=true
# hi from comment
host="localhost"
wello
)";

    std::cout << "\nParsing input:\n" << input << std::endl;

    auto result = schema.parse(input);
    if (result.hasErrors()) result.printErrors();
    else {
        std::cout << "Parsed values:" << std::endl;
        std::cout << "port: " << result.get("port").as<int>() << std::endl;
        std::cout << "host: " << result.get("host").as<std::string>() << std::endl;
        std::cout << "debug: " << (result.get("debug").as<bool>() ? "true" : "false") << std::endl;
    }

    return 0;
}