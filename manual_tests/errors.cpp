#include "../src/cord.hpp"

#include <iostream>

int main() {
    cord::Schema schema;
    schema.setAllowComments(true);
    schema.setStrict(true);

    // Fields that should exist but are missing or malformed
    schema.add<bool>("not_in_config").required();
    schema.add<bool>("empty").required();
    schema.add<bool>("boolean").required();

    // Number parsing errors
    schema.add<int>("int").required();
    schema.add<int>("int2").required();
    schema.add<int>("int3").required();
    schema.add<float>("float").required();
    schema.add<double>("double").required();

    // String parsing errors
    schema.add<std::string>("string").required();
    schema.add<std::string>("string2").required();

    // Vector parsing errors
    schema.add<std::vector<int>>("badvec").required();
    schema.add<std::vector<int>>("worsevec").required();
    schema.add<std::vector<std::string>>("whatthehell").required();
    schema.add<std::vector<int>>("mixed").required();
    schema.add<std::vector<std::string>>("forgotquotes").required();
    schema.add<std::vector<int>>("emptyvec").required();
    schema.add<std::vector<int>>("trailingcomma").required();
    schema.add<std::vector<int>>("noclosing").required();

    schema.describe();
    std::cout << std::endl;

    auto result = schema.parseFile("manual_tests/errors.conf");
    if (result.hasErrors()) {
        std::cout << "Found errors:\n";
        result.printErrors();
    }

    return 0;
}