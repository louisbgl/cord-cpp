#include "cord.hpp"

#include <iostream>
#include <vector>

// I recommend peeking at examples/simplest/simplest.cpp first :)

// cord supports custom struct types in addition to primitives
// This lets you parse config files with structured data that maps directly to your C++ types
// Custom structs can contain any combination of bool, int, float, double, string, and vector<T>
// Nesting custom structs inside other custom structs is not yet supported

// Define your struct as usual
// All fields must have default values for default-construction
struct Artist {
    std::string name = "";
    int year_formed = 0;
    std::string genre = "";
};

// Register the struct with cord's type system
// Without this macro, using cord::CustomStruct<Artist> will trigger a compile-time error
CORD_REGISTER_STRUCT(Artist);

// Define another struct for music tracks
// You can use different primitive types in the same struct
struct Track {
    std::string title = "";
    int duration_seconds = 0;
    double rating = 0.0;
    bool explicit_lyrics = false;
};
CORD_REGISTER_STRUCT(Track);


int main() {
    // Create a CustomStruct definition for Artist
    // This tells cord what fields exist and how to access them
    cord::CustomStruct<Artist> artist_def("Artist");

    // Map each struct field to its config key using member pointers
    // You can apply constraints like required(), default_(), min(), max() just like primitive fields
    artist_def.add("name", &Artist::name).required();
    artist_def.add("year_formed", &Artist::year_formed).min(1900).max(2024);
    artist_def.add("genre", &Artist::genre).default_("Unknown");

    // Set up the Track struct definition
    cord::CustomStruct<Track> track_def("Track");
    track_def.add("title", &Track::title).required();
    track_def.add("duration_seconds", &Track::duration_seconds).min(1);
    track_def.add("rating", &Track::rating).min(0.0).max(5.0).default_(3.0);
    track_def.add("explicit_lyrics", &Track::explicit_lyrics).default_(false);

    // Create a schema like usual
    cord::Schema schema;

    // Add a primitive field
    schema.add<std::string>("library_name").required();

    // Add a single custom struct field
    // Pass the field name and the CustomStruct definition you created earlier
    schema.add<Artist>("featured_artist", artist_def);

    // Add a vector of custom structs
    // This lets you parse an array of structured objects from your config file
    // Use the same track_def for all elements in the vector
    schema.add<std::vector<Track>>("tracks", track_def);

    schema.describe();
    std::cout << std::endl;

    auto result = schema.parseFile("examples/custom_types/custom_types.conf");

    if (result.hasErrors()) {
        result.printErrors();
        return 1;
    }

    std::cout << "Parsed values:" << std::endl;
    std::cout << "library_name: " << result.get("library_name").as<std::string>() << std::endl;
    std::cout << std::endl;

    // Get a single custom struct using .as<Artist>()
    // This works the same as getting primitive types
    Artist artist = result.get("featured_artist").as<Artist>();
    std::cout << "Featured Artist:" << std::endl;
    std::cout << "  name:        " << artist.name << std::endl;
    std::cout << "  year_formed: " << artist.year_formed << std::endl;
    std::cout << "  genre:       " << artist.genre << std::endl;
    std::cout << std::endl;

    // Get a vector of custom structs using .as<std::vector<Track>>()
    std::vector<Track> tracks = result.get("tracks").as<std::vector<Track>>();
    std::cout << "Tracks (" << tracks.size() << "):" << std::endl;
    for (size_t i = 0; i < tracks.size(); ++i) {
        std::cout << "  [" << i << "] " << tracks[i].title << std::endl;
        std::cout << "      duration: " << tracks[i].duration_seconds << "s" << std::endl;
        std::cout << "      rating:   " << tracks[i].rating << "/5.0" << std::endl;
        std::cout << "      explicit: " << (tracks[i].explicit_lyrics ? "yes" : "no") << std::endl;
    }

    return 0;
}
