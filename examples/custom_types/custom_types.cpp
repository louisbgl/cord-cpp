#include "cord.hpp"

#include <iostream>

struct Sound {
    std::string name = "";
    int volume = 0;
};
CORD_REGISTER_STRUCT(Sound);


int main(void) {
    cord::CustomStruct<Sound> sound_def("Sound");
    sound_def.add("name", &Sound::name);
    sound_def.add("volume", &Sound::volume);

    cord::Schema schema;
    schema.add<std::string>("title");
    schema.add<Sound>("sound", sound_def);

    auto result = schema.parseFile("examples/custom_types/custom_types.conf");
    if (result.hasErrors()) {
        result.printErrors();
        return 1;
    }
    
    std::cout << "Parsed values:" << std::endl;
    std::cout << "title: " << result.get("title").as<std::string>() << std::endl;
    Sound sound = result.get("sound").as<Sound>();
    std::cout << "sound.name: " << sound.name << std::endl;
    std::cout << "sound.volume: " << sound.volume << std::endl;
    return 0;
}