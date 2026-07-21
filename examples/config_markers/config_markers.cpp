#include "../../cord.hpp"

#include <iostream>

// I recommend peeking at examples/simplest/simplest.cpp first :)

// This example is th same as examples/simplest/simplest.cpp, but with a custom comment marker defined

int main() {
    cord::Schema schema;
    schema.setAllowComments(true);

    // Here we can define our own comment marker, with '#' being the default
    // You can do single chars, as well as multi-char strings, like '//' or '--'
    schema.setCommentMarker("--");

    schema.add<int>("port").required();
    schema.add<std::string>("host").default_("localhost");
    schema.add<bool>("debug").default_(false);

    auto result = schema.parseFile("examples/config_markers/config_markers.conf");

    if (result.hasErrors()) result.printErrors();
    else {
        std::cout << "Parsed values:" << std::endl;
        std::cout << "port: " << result.get("port").as<int>()                         << std::endl;
        std::cout << "host: " << result.get("host").as<std::string>()                 << std::endl;
        std::cout << "debug: " << (result.get("debug").as<bool>() ? "true" : "false") << std::endl;
    }

    return 0;
}