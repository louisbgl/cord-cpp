#include "cord.hpp"

#include <iostream>
#include <vector>

// I recommend peeking at examples/simplest/simplest.cpp first :)

// As seen in previous examples, cord supports parsing primitive types only
// But.. you may provide a custom struct, made of primitive types, and given a little bit of setup
// cord can parse just about anything
// Note: This is currently limited to default-constructible structs
// Note: This is currently limited to structs containing only primitive types, custom struct nesting is not yet supported

// Here we define a struct, as one does
struct Artist {
    std::string name = "";
    int age = 0;
    int num_albums = 0;
};

// Here, you tell cord about your struct
// If you dont register a struct with CORD_REGISTER_STRUCT
// you will get a compile-time error when calling cord::CustomStruct<Artist>
CORD_REGISTER_STRUCT(Artist);

// Another custom struct, why not
struct Sound {
    std::string name = "";
    int duration = 0;
};
CORD_REGISTER_STRUCT(Sound);


int main(void) {
    // Here we define a cord::CustomStruct for our Artist struct
    cord::CustomStruct<Artist> artist_def("Artist");
    
    // To tell cord about the fields in your struct
    // you call add() on the CustomStruct, passing the field name and a pointer to the field in your struct
    artist_def.add("name", &Artist::name);
    artist_def.add("age", &Artist::age);
    artist_def.add("num_albums", &Artist::num_albums);

    // Same setup for our Sound struct
    cord::CustomStruct<Sound> sound_def("Sound");
    sound_def.add("name", &Sound::name);
    sound_def.add("duration", &Sound::duration);


    // Create a Schema, like usual
    cord::Schema schema;
    schema.add<std::string>("album");

    // Then when you want to add a field of your custom type
    // you call add() on the Schema, passing the field name and the CustomStruct you defined earlier
    schema.add<Artist>("artist", artist_def);

    schema.add<std::vector<Sound>>("sounds", sound_def);

    schema.describe();
    std::cout << std::endl;

    auto result = schema.parseFile("examples/custom_types/custom_types.conf");
    if (result.hasErrors()) {
        result.printErrors();
        return 1;
    }
    
    std::cout << "Parsed values:" << std::endl;
    std::cout << "album: " << result.get("album").as<std::string>() << std::endl;
    std::cout << "artist.name: " << result.get("artist").as<Artist>().name << std::endl;
    std::cout << "artist.age: " << result.get("artist").as<Artist>().age << std::endl;
    std::cout << "artist.num_albums: " << result.get("artist").as<Artist>().num_albums << std::endl;
    std::vector<Sound> sounds = result.get("sounds").as<std::vector<Sound>>();
    for (size_t i = 0; i < sounds.size(); ++i) {
        const Sound& sound = sounds[i];
        std::cout << "sound[" << i << "].name: " << sound.name << std::endl;
        std::cout << "sound[" << i << "].duration: " << sound.duration << std::endl;
    }
    return 0;
}