#include "cord.hpp"

#include <iostream>

// The smallest possible use of cord: one struct, one describe, 3 lines to parse
// Errors are not reported here on purpose, a bad config just exits with 1
// See the error handling example for how to report them

// This is your own config struct, cord fills it in
// It needs to be default constructible, the defaults are kept for absent keys
struct Config {
    int port = 1234;
    std::string log_path = {};
    bool verbose = false;
};

// Cord needs to know which key goes into which member
// That's what describe is for: one add per member, declared before parsing
void describe(cord::Struct<Config>& s) {
    s.add("port", &Config::port);
    s.add("log_path", &Config::log_path);
    s.add("verbose", &Config::verbose);
}

void printConfig(const Config& conf) {
    std::cout << "Config(" <<
    "port=" << conf.port <<
    ", log_path=`" << conf.log_path <<
    "`, verbose=" << std::boolalpha << conf.verbose <<
    ")" << std::endl;
}

int main() {
    // The 3 lines needed to get a Config out of a .cord file
    // The path is relative to where you run from, here the repo root
    auto result = cord::parseFile<Config>("examples/basic/basic.cord");
    Config conf{};
    if (!result.get(conf)) return 1;
    // From here on, conf is valid: every key was read or kept its default

    // Printing the default and the parsed config to see what cord changed
    std::cout << "Default Config:\n";
    Config c{};
    printConfig(c);

    std::cout << "Parsed Config:\n";
    printConfig(conf);
    
    return 0;
}
