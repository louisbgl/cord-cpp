#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <vector>

#include "cord.hpp"
#include "test_helpers.hpp"

namespace {

struct Server {
    std::string name = "default";
    int port = 8080;
    bool verbose = false;
};

void describe(cord::Struct<Server>& s) {
    s.add("name", &Server::name);
    s.add("port", &Server::port);
    s.add("verbose", &Server::verbose);
}

// The grammar's example config, bound end to end.
struct Pos {
    float x = 0;
    float y = 0;
};

struct Player {
    std::string name;
    Pos pos;
};

struct Entity {
    std::string name;
    int hp = 100;
};

struct Example {
    std::string name;
    int port = 0;
    double ratio = 0;
    bool verbose = true;
    std::vector<int> empty = {1};
    std::vector<std::string> tags;
    std::vector<std::vector<int>> grid;
    Player player;
    std::vector<Entity> entities;
};

void describe(cord::Struct<Pos>& s) {
    s.add("x", &Pos::x);
    s.add("y", &Pos::y);
}

void describe(cord::Struct<Player>& s) {
    s.add("name", &Player::name);
    s.add("pos", &Player::pos);
}

void describe(cord::Struct<Entity>& s) {
    s.add("name", &Entity::name);
    s.add("hp", &Entity::hp);
}

void describe(cord::Struct<Example>& s) {
    s.add("name", &Example::name);
    s.add("port", &Example::port);
    s.add("ratio", &Example::ratio);
    s.add("verbose", &Example::verbose);
    s.add("empty", &Example::empty);
    s.add("tags", &Example::tags);
    s.add("grid", &Example::grid);
    s.add("player", &Example::player);
    s.add("entities", &Example::entities);
}

/**
 * @brief Prints a result's diagnostics for a failure message.
 */
template <class T>
std::string trace(std::string_view source, const cord::Result<T>& result) {
    std::string text = "input: " + show(source) + "\nvalue: " + (result.value ? "present" : "empty") + "\ndiagnostics:";
    for (const cord::Diagnostic& diagnostic : result.diagnostics) text += "\n  " + cord::toString(diagnostic);
    return text;
}

/** Each diagnostic as `error 1:5 port`, in order. */
std::vector<std::string> summary(const std::vector<cord::Diagnostic>& diagnostics) {
    std::vector<std::string> lines;
    for (const cord::Diagnostic& diagnostic : diagnostics) {
        lines.push_back((diagnostic.severity == cord::Severity::Error ? "error " : "warning ")
            + std::to_string(diagnostic.line) + ":" + std::to_string(diagnostic.column)
            + (diagnostic.path.empty() ? "" : " " + diagnostic.path));
    }
    return lines;
}

} // namespace

// Macros, not functions: a failure must point at the test's line, not at a helper's.

/** Parses the source as T, leaving `result` to check. */
#define PARSE(T, source)                                                    \
    const std::string_view parsed_source = (source);                        \
    const cord::Result<T> result = cord::parse<T>(parsed_source);           \
    INFO(trace(parsed_source, result))

TEST_CASE("A valid config", "[cord]") {
    PARSE(Server, "name = \"cord\"\nport = 1\nverbose = true");

    SECTION("has a value and nothing to report") {
        REQUIRE(result.value.has_value());
        CHECK(result.diagnostics.empty());
        CHECK_FALSE(result.hasErrors());
        CHECK_FALSE(result.hasWarnings());
    }
    SECTION("the value holds the config") {
        REQUIRE(result.value.has_value());
        CHECK(result.value->name == "cord");
        CHECK(result.value->port == 1);
        CHECK(result.value->verbose == true);
    }
    SECTION("get copies it out") {
        Server server;
        CHECK(result.get(server));
        CHECK(server.name == "cord");
        CHECK(server.port == 1);
    }
}

TEST_CASE("Warnings keep the result usable", "[cord]") {
    PARSE(Server, "port = 1\nextra = 2");

    SECTION("the value is there") {
        REQUIRE(result.value.has_value());
        CHECK(result.value->port == 1);
        CHECK(result.value->name == "default");
    }
    SECTION("the warnings are reported with it") {
        CHECK(summary(result.diagnostics) == std::vector<std::string>{
            "warning 2:9 extra", "warning 0:0 name", "warning 0:0 verbose"});
        CHECK(result.hasWarnings());
        CHECK_FALSE(result.hasErrors());
    }
    SECTION("get still works") {
        Server server;
        CHECK(result.get(server));
        CHECK(server.port == 1);
    }
}

TEST_CASE("A syntax error", "[cord]") {
    PARSE(Server, "name = \"cord\"\nport = @\nverbose = true");

    SECTION("gives no value") {
        CHECK_FALSE(result.value.has_value());
        CHECK(result.hasErrors());
    }
    SECTION("skips binding, so only syntax errors are reported") {
        CHECK(summary(result.diagnostics) == std::vector<std::string>{"error 2:8"});
    }
    SECTION("get fails and leaves its argument alone") {
        Server server;
        server.port = 42;
        CHECK_FALSE(result.get(server));
        CHECK(server.port == 42);
    }
}

TEST_CASE("Several syntax errors are all reported", "[cord]") {
    PARSE(Server, "name = @\nport = 1\nverbose = @");
    CHECK_FALSE(result.value.has_value());
    CHECK(summary(result.diagnostics) == std::vector<std::string>{"error 1:8", "error 3:11"});
}

TEST_CASE("A binding error", "[cord]") {
    PARSE(Server, "port = \"8080\"\nextra = 1");

    SECTION("gives no value") {
        CHECK_FALSE(result.value.has_value());
        CHECK(result.hasErrors());
    }
    SECTION("reports the warnings found along with it") {
        CHECK(summary(result.diagnostics) == std::vector<std::string>{
            "warning 2:9 extra", "warning 0:0 name", "error 1:8 port", "warning 0:0 verbose"});
        CHECK(result.hasWarnings());
    }
    SECTION("get fails") {
        Server server;
        CHECK_FALSE(result.get(server));
    }
}

TEST_CASE("An empty config", "[cord]") {
    PARSE(Server, "");
    REQUIRE(result.value.has_value());
    CHECK(result.value->port == 8080);
    CHECK(summary(result.diagnostics) == std::vector<std::string>{
        "warning 0:0 name", "warning 0:0 port", "warning 0:0 verbose"});
}

TEST_CASE("The grammar's example config, bound", "[cord][integration]") {
    PARSE(Example,
        "# comments run to end of line\n"
        "name    = \"cord\"            # string, double quotes only\n"
        "port    = 8080              # int\n"
        "ratio   = -0.75             # float, needs digits on both sides of the dot\n"
        "verbose = false             # bool, lowercase only\n"
        "empty   = []                # empty array\n"
        "\n"
        "tags = [\"fast\", \"small\",]   # trailing comma allowed\n"
        "grid = [[1, 2], [3, 4]]     # arrays nest\n"
        "\n"
        "player = { name = \"bob\", pos = { x = 1.5, y = -2.0 } }\n"
        "\n"
        "# arrays and objects can span lines; commas still separate elements\n"
        "entities = [\n"
        "    { name = \"a\", hp = 10 },\n"
        "    { name = \"b\", hp = 20 },\n"
        "]\n");

    REQUIRE(result.value.has_value());
    CHECK(result.diagnostics.empty());

    const Example& config = *result.value;
    CHECK(config.name == "cord");
    CHECK(config.port == 8080);
    CHECK(config.ratio == -0.75);
    CHECK(config.verbose == false);
    CHECK(config.empty.empty());
    CHECK(config.tags == std::vector<std::string>{"fast", "small"});
    CHECK(config.grid == std::vector<std::vector<int>>{{1, 2}, {3, 4}});
    CHECK(config.player.name == "bob");
    CHECK(config.player.pos.x == 1.5f);
    CHECK(config.player.pos.y == -2.0f);
    REQUIRE(config.entities.size() == 2);
    CHECK(config.entities[1].name == "b");
    CHECK(config.entities[1].hp == 20);
}

TEST_CASE("Diagnostics print for people", "[cord][format]") {
    using cord::Diagnostic;
    using cord::Severity;

    SECTION("with a position and a path") {
        CHECK(cord::toString(Diagnostic{Severity::Error, 1, 5, "port", "Expected an integer"})
            == "[CORD] Error 1:5 [port]: Expected an integer");
    }
    SECTION("a syntax error has no path") {
        CHECK(cord::toString(Diagnostic{Severity::Error, 2, 8, "", "Unexpected character: '@'"})
            == "[CORD] Error 2:8: Unexpected character: '@'");
    }
    SECTION("an absent key has no position") {
        CHECK(cord::toString(Diagnostic{Severity::Warning, 0, 0, "motd", "Absent key, keeping its default"})
            == "[CORD] Warning [motd]: Absent key, keeping its default");
    }
    SECTION("a nested path") {
        CHECK(cord::toString(Diagnostic{Severity::Warning, 3, 12, "entities[0].pos.z", "Unknown key, skipped"})
            == "[CORD] Warning 3:12 [entities[0].pos.z]: Unknown key, skipped");
    }
}
