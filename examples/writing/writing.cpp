#include "cord.hpp"

#include <iostream>

// I recommend peeking at examples/simplest/simplest.cpp first :)

// cord results are not read-only, you can modify values and write them back to disk

int main() {
    cord::Schema schema;
    schema.add<std::string>("player_name").required();
    schema.add<int>("high_score").default_(0);
    schema.add<int>("level").default_(1);
    schema.add<bool>("sound_enabled").default_(true);

    // parse an existing save file
    auto result = schema.parseFile("save.conf");
    if (result.hasErrors()) {
        result.printErrors();
        return 1;
    }

    std::cout << "Loaded save:" << std::endl;
    std::cout << result.write() << std::endl;

    // simulate a game session: update score and level
    int new_score = result.get("high_score").as<int>() + 500;
    int new_level = result.get("level").as<int>() + 1;

    result.set("high_score", new_score)
          .set("level", new_level)
          .set("sound_enabled", false);

    std::cout << "After session:" << std::endl;
    std::cout << result.write() << std::endl;

    // write to a new file (so save.conf stays intact for re-runs)
    result.writeFile("new_save.conf");
    std::cout << "Save written to new_save.conf" << std::endl;

    return 0;
}
