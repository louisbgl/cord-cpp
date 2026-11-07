#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "context.hpp"
#include "parser.hpp"
#include "struct.hpp"
#include "test_helpers.hpp"

using cord::Context;
using cord::Converter;
using cord::Node;
using cord::Parser;
using cord::Struct;

namespace {

struct Pair {
    int a = 1;
    std::string b = "initial";
};

void describe(Struct<Pair>& s) {
    s.add("a", &Pair::a);
    s.add("b", &Pair::b);
}

struct Config {
    int port = 8080;
    int retries = 3;
    std::string name = "default";
    double ratio = 0.5;
    bool verbose = false;
};

void describe(Struct<Config>& s) {
    s.add("port", &Config::port);
    s.add("retries", &Config::retries);
    s.add("name", &Config::name);
    s.add("ratio", &Config::ratio);
    s.add("verbose", &Config::verbose);
}

struct Empty {};

void describe(Struct<Empty>&) {}

struct Pos {
    double x = 0.5;
    double y = 0.5;
};

void describe(Struct<Pos>& s) {
    s.add("x", &Pos::x);
    s.add("y", &Pos::y);
}

struct Optionals {
    std::optional<std::string> motd;
    std::optional<int> port;
    std::optional<int> workers = 4;
    std::optional<Pos> pos;
    int plain = 1;
};

void describe(Struct<Optionals>& s) {
    s.add("motd", &Optionals::motd);
    s.add("port", &Optionals::port);
    s.add("workers", &Optionals::workers);
    s.add("pos", &Optionals::pos);
    s.add("plain", &Optionals::plain);
}

struct Route {
    std::vector<Pos> points;
};

void describe(Struct<Route>& s) {
    s.add("points", &Route::points);
}

struct Player {
    std::string name = "nobody";
    Pos pos;
    int hp = 100;
};

void describe(Struct<Player>& s) {
    s.add("name", &Player::name);
    s.add("pos", &Player::pos);
    s.add("hp", &Player::hp);
}

struct World {
    Player player;
    Pos origin;
    Pos spawn;
};

void describe(Struct<World>& s) {
    s.add("player", &World::player);
    s.add("origin", &World::origin);
    s.add("spawn", &World::spawn);
}

struct Counted {
    int a = 0;
};

int counted_describes = 0;

void describe(Struct<Counted>& s) {
    counted_describes++;
    s.add("a", &Counted::a);
}

struct Undescribed {
    int a = 0;
};

template <class T>
struct Bound {
    T out{};
    bool ok = false;
    Context ctx;
};

/**
 * @brief Parses the source, then binds its tree to T through T's describe().
 */
template <class T>
Bound<T> bind(std::string_view source) {
    const cord::ParseResult parsed = Parser(source).parse();
    INFO(show(source));
    REQUIRE(parsed.diagnostics.empty());

    Struct<T> description;
    describe(description);

    Bound<T> bound;
    bound.ok = description.read(parsed.tree, bound.out, bound.ctx);
    return bound;
}

/**
 * @brief Sums up diagnostics as `error 1:5 port`, sorted: their order between keys is not part of the contract.
 */
std::vector<std::string> summary(const Context& ctx) {
    std::vector<std::string> lines;
    for (const cord::Diagnostic& diagnostic : ctx.diagnostics()) {
        lines.push_back((diagnostic.severity == cord::Severity::Error ? "error " : "warning ")
            + std::to_string(diagnostic.line) + ":" + std::to_string(diagnostic.column) + " " + diagnostic.path);
    }
    std::sort(lines.begin(), lines.end());
    return lines;
}

/**
 * @brief Prints diagnostics with their messages for a failure message.
 */
std::string trace(std::string_view source, const Context& ctx) {
    std::string text = "input: " + show(source) + "\ndiagnostics:";
    for (const cord::Diagnostic& diagnostic : ctx.diagnostics()) {
        text += "\n  " + std::to_string(diagnostic.line) + ":" + std::to_string(diagnostic.column)
              + " [" + diagnostic.path + "] " + diagnostic.message;
    }
    return text;
}

/** Whether add() accepts a member of type M. */
template <class T, class M>
concept CanAdd = requires(Struct<T>& s, M T::* field) { s.add("key", field); };

} // namespace

// Macros, not functions: a failure must point at the test's line, not at a helper's.

/** Binds the source: read returned `expected_ok`, and these are all the diagnostics. Leaves `bound` to check. */
#define BIND(T, source, expected_ok, ...)                                   \
    const Bound<T> bound = bind<T>(source);                                 \
    INFO(trace(source, bound.ctx));                                         \
    CHECK(bound.ok == (expected_ok));                                       \
    CHECK(summary(bound.ctx) == std::vector<std::string>{__VA_ARGS__})

TEST_CASE("Keys and member types", "[struct]") {
    STATIC_CHECK(CanAdd<Pair, int>);
    STATIC_CHECK(CanAdd<Pair, std::string>);
    STATIC_CHECK(CanAdd<Pair, double>);
    STATIC_CHECK(CanAdd<Pair, bool>);
}

TEST_CASE("Present keys", "[struct][present]") {
    SECTION("each value goes to its own member") {
        BIND(Config, "port = 1\nretries = 2\nname = \"x\"\nratio = 2.5\nverbose = true", true);
        CHECK(bound.out.port == 1);
        CHECK(bound.out.retries == 2);
        CHECK(bound.out.name == "x");
        CHECK(bound.out.ratio == 2.5);
        CHECK(bound.out.verbose == true);
    }
    SECTION("the order in the config does not matter") {
        BIND(Pair, "b = \"y\"\na = 2", true);
        CHECK(bound.out.a == 2);
        CHECK(bound.out.b == "y");
    }
    SECTION("each member's converter is used") {
        BIND(Config, "port = 1\nretries = 2\nname = \"x\"\nratio = 1\nverbose = true", true);
        CHECK(bound.out.ratio == 1.0);
    }
}

TEST_CASE("Absent keys", "[struct][absent]") {
    SECTION("warn with no position, and keep the initializer") {
        BIND(Pair, "a = 2", true, "warning 0:0 b");
        CHECK(bound.out.a == 2);
        CHECK(bound.out.b == "initial");
    }
    SECTION("each one warns") {
        BIND(Pair, "", true, "warning 0:0 a", "warning 0:0 b");
        CHECK(bound.out.a == 1);
        CHECK(bound.out.b == "initial");
    }
    SECTION("keys are case sensitive") {
        BIND(Pair, "A = 2\nb = \"y\"", true, "warning 0:0 a", "warning 1:5 A");
        CHECK(bound.out.a == 1);
    }
}

TEST_CASE("Unknown keys", "[struct][unknown]") {
    SECTION("warn at their value, and change nothing") {
        BIND(Pair, "a = 2\nb = \"y\"\nc = 3", true, "warning 3:5 c");
        CHECK(bound.out.a == 2);
        CHECK(bound.out.b == "y");
    }
    SECTION("any value is fine, nothing converts it") {
        BIND(Pair, "a = 2\nb = \"y\"\nc = [1, \"two\"]\nd = { e = 1 }", true, "warning 3:5 c", "warning 4:5 d");
    }
    SECTION("repeated, each one warns as unknown") {
        BIND(Pair, "a = 2\nb = \"y\"\nc = 1\nc = 2", true, "warning 3:5 c", "warning 4:5 c");
    }
    SECTION("a struct with no keys") {
        BIND(Empty, "a = 1", true, "warning 1:5 a");
    }
}

TEST_CASE("Duplicate keys", "[struct][duplicate]") {
    SECTION("the last one wins, and the repeat warns") {
        BIND(Pair, "a = 2\na = 3\nb = \"y\"", true, "warning 2:5 a");
        CHECK(bound.out.a == 3);
    }
    SECTION("each repeat warns") {
        BIND(Pair, "a = 2\na = 3\na = 4\nb = \"y\"", true, "warning 2:5 a", "warning 3:5 a");
        CHECK(bound.out.a == 4);
    }
    SECTION("an overridden value is never converted") {
        BIND(Pair, "a = \"not an int\"\na = 3\nb = \"y\"", true, "warning 2:5 a");
        CHECK(bound.out.a == 3);
    }
    SECTION("a bad last value is an error, even after a good one") {
        BIND(Pair, "a = 2\na = \"not an int\"\nb = \"y\"", false, "error 2:5 a", "warning 2:5 a");
    }
}

TEST_CASE("Bad values", "[struct][bad]") {
    SECTION("fail the read, with the key as the path") {
        BIND(Pair, "a = \"not an int\"\nb = \"y\"", false, "error 1:5 a");
    }
    SECTION("the other keys are still read") {
        BIND(Pair, "a = \"not an int\"\nb = \"y\"", false, "error 1:5 a");
        CHECK(bound.out.b == "y");
    }
    SECTION("each one is reported") {
        BIND(Pair, "a = \"not an int\"\nb = 2", false, "error 1:5 a", "error 2:5 b");
    }
    SECTION("warnings are reported along with errors") {
        BIND(Pair, "a = \"not an int\"\nc = 1", false, "error 1:5 a", "warning 0:0 b", "warning 2:5 c");
    }
}

TEST_CASE("Which structs can be converted", "[struct][nested]") {
    STATIC_CHECK(cord::Convertible<Pos>);
    STATIC_CHECK(cord::Convertible<Player>);
    STATIC_CHECK(CanAdd<World, Player>);
    STATIC_CHECK_FALSE(cord::Convertible<Undescribed>);
    STATIC_CHECK_FALSE(cord::Describable<int>);
    STATIC_CHECK_FALSE(cord::Describable<double>);
    STATIC_CHECK_FALSE(cord::Describable<Undescribed>);
}

TEST_CASE("Nested structs", "[struct][nested]") {
    SECTION("values reach the inner members") {
        BIND(Player, "name = \"bob\"\npos = { x = 1.5, y = 2.5 }\nhp = 5", true);
        CHECK(bound.out.name == "bob");
        CHECK(bound.out.pos.x == 1.5);
        CHECK(bound.out.pos.y == 2.5);
        CHECK(bound.out.hp == 5);
    }
    SECTION("over several lines") {
        BIND(Player, "name = \"bob\"\npos = {\n  x = 1.5,\n  y = 2.5,\n}\nhp = 5", true);
        CHECK(bound.out.pos.y == 2.5);
    }
    SECTION("several levels") {
        BIND(World, "player = { name = \"bob\", pos = { x = 1.5, y = 2.5 }, hp = 5 }\n"
                    "origin = { x = 3.5, y = 4.5 }\nspawn = { x = 5.5, y = 6.5 }", true);
        CHECK(bound.out.player.name == "bob");
        CHECK(bound.out.player.pos.y == 2.5);
    }
    SECTION("two members of the same struct type stay separate") {
        BIND(World, "player = { name = \"bob\", pos = { x = 1.5, y = 2.5 }, hp = 5 }\n"
                    "origin = { x = 3.5, y = 4.5 }\nspawn = { x = 5.5, y = 6.5 }", true);
        CHECK(bound.out.origin.x == 3.5);
        CHECK(bound.out.spawn.x == 5.5);
    }
}

TEST_CASE("Nested structs report like the top level", "[struct][nested]") {
    SECTION("a bad value has the full path") {
        BIND(Player, "name = \"bob\"\npos = { x = \"oops\", y = 2.5 }\nhp = 5", false, "error 2:13 pos.x");
    }
    SECTION("through several levels") {
        BIND(World, "player = { name = \"bob\", pos = { x = 1.5, y = true }, hp = 5 }\n"
                    "origin = { x = 3.5, y = 4.5 }\nspawn = { x = 5.5, y = 6.5 }", false, "error 1:47 player.pos.y");
    }
    SECTION("the other values are still read") {
        BIND(Player, "name = \"bob\"\npos = { x = \"oops\", y = 2.5 }\nhp = 5", false, "error 2:13 pos.x");
        CHECK(bound.out.pos.y == 2.5);
        CHECK(bound.out.hp == 5);
    }
    SECTION("an absent inner key warns, and keeps its initializer") {
        BIND(Player, "name = \"bob\"\npos = { x = 1.5 }\nhp = 5", true, "warning 0:0 pos.y");
        CHECK(bound.out.pos.x == 1.5);
        CHECK(bound.out.pos.y == 0.5);
    }
    SECTION("an absent object warns once, not once per inner key") {
        BIND(Player, "name = \"bob\"\nhp = 5", true, "warning 0:0 pos");
        CHECK(bound.out.pos.x == 0.5);
    }
    SECTION("an unknown inner key warns") {
        BIND(Player, "name = \"bob\"\npos = { x = 1.5, y = 2.5, z = 3.5 }\nhp = 5", true, "warning 2:31 pos.z");
    }
    SECTION("a duplicate inner key warns, and the last one wins") {
        BIND(Player, "name = \"bob\"\npos = { x = 1.5, x = 2.5, y = 3.5 }\nhp = 5", true, "warning 2:22 pos.x");
        CHECK(bound.out.pos.x == 2.5);
    }
    SECTION("an empty object warns for each key") {
        BIND(Player, "name = \"bob\"\npos = {}\nhp = 5", true, "warning 0:0 pos.x", "warning 0:0 pos.y");
    }
}

TEST_CASE("A struct member needs an object", "[struct][nested]") {
    SECTION("anything else is refused at the value") {
        BIND(Player, "name = \"bob\"\npos = 1\nhp = 5", false, "error 2:7 pos");
    }
    SECTION("an array is not an object") {
        BIND(Player, "name = \"bob\"\npos = [1.5, 2.5]\nhp = 5", false, "error 2:7 pos");
    }
    SECTION("nothing inside it gets reported") {
        BIND(Player, "name = \"bob\"\npos = \"nope\"\nhp = 5", false, "error 2:7 pos");
        CHECK(bound.out.hp == 5);
    }
}

TEST_CASE("Vectors of structs", "[struct][nested][vector]") {
    SECTION("each element is read as a struct") {
        BIND(Route, "points = [{ x = 1.5, y = 2.5 }, { x = 3.5, y = 4.5 }]", true);
        REQUIRE(bound.out.points.size() == 2);
        CHECK(bound.out.points[0].y == 2.5);
        CHECK(bound.out.points[1].x == 3.5);
    }
    SECTION("an empty array") {
        BIND(Route, "points = []", true);
        CHECK(bound.out.points.empty());
    }
    SECTION("a bad inner value has the index in its path") {
        BIND(Route, "points = [{ x = 1.5, y = 2.5 }, { x = \"oops\", y = 1.5 }]", false, "error 1:39 points[1].x");
    }
    SECTION("an absent inner key warns with the index in its path") {
        BIND(Route, "points = [{ x = 1.5 }]", true, "warning 0:0 points[0].y");
        REQUIRE(bound.out.points.size() == 1);
        CHECK(bound.out.points[0].y == 0.5);
    }
    SECTION("an unknown inner key warns with the index in its path") {
        BIND(Route, "points = [{ x = 1.5, y = 2.5, z = 1 }]", true, "warning 1:35 points[0].z");
    }
    SECTION("an element that is not an object") {
        BIND(Route, "points = [1]", false, "error 1:11 points[0]");
    }
}

TEST_CASE("Optional members", "[struct][optional]") {
    SECTION("absent, they stay silent and empty") {
        BIND(Optionals, "plain = 2", true);
        CHECK_FALSE(bound.out.motd.has_value());
        CHECK_FALSE(bound.out.port.has_value());
        CHECK_FALSE(bound.out.pos.has_value());
    }
    SECTION("absent, an initializer is kept like for any member") {
        BIND(Optionals, "plain = 2", true);
        CHECK(bound.out.workers == 4);
    }
    SECTION("only plain members warn when absent") {
        BIND(Optionals, "", true, "warning 0:0 plain");
    }
    SECTION("present, they hold the value") {
        BIND(Optionals, "motd = \"hi\"\nport = 80\nworkers = 8\npos = { x = 1.5, y = 2.5 }\nplain = 2", true);
        CHECK(bound.out.motd == "hi");
        CHECK(bound.out.port == 80);
        CHECK(bound.out.workers == 8);
        REQUIRE(bound.out.pos.has_value());
        CHECK(bound.out.pos->y == 2.5);
    }
    SECTION("a bad value is an error") {
        BIND(Optionals, "port = \"80\"\nplain = 2", false, "error 1:8 port");
    }
    SECTION("unknown and duplicate keys still warn") {
        BIND(Optionals, "port = 80\nport = 81\nplain = 2\nextra = 1", true, "warning 2:8 port", "warning 4:9 extra");
        CHECK(bound.out.port == 81);
    }
    SECTION("an optional struct follows the struct rules inside") {
        BIND(Optionals, "pos = { x = \"oops\", y = 2.5 }\nplain = 2", false, "error 1:13 pos.x");
    }
    SECTION("an absent key inside an optional struct still warns") {
        BIND(Optionals, "pos = { x = 1.5 }\nplain = 2", true, "warning 0:0 pos.y");
        REQUIRE(bound.out.pos.has_value());
        CHECK(bound.out.pos->y == 0.5);
    }
}

TEST_CASE("describe() runs once per type", "[struct][nested]") {
    const cord::ParseResult parsed = Parser("a = 1").parse();
    REQUIRE(parsed.tree.size() == 1);
    const Node& node = parsed.tree[0].value;

    const cord::ParseResult object = Parser("o = { a = 1 }").parse();
    REQUIRE(object.tree.size() == 1);

    Context ctx;
    Counted out;
    CHECK(Converter<Counted>::read(object.tree[0].value, out, ctx));
    CHECK(Converter<Counted>::read(object.tree[0].value, out, ctx));
    CHECK_FALSE(Converter<Counted>::read(node, out, ctx));
    CHECK(out.a == 1);
    CHECK(counted_describes == 1);
}

TEST_CASE("Paths", "[struct][path]") {
    const cord::ParseResult parsed = Parser("a = \"not an int\"\nc = 1\nc = 2").parse();
    REQUIRE(parsed.diagnostics.empty());
    Struct<Pair> description;
    describe(description);

    Pair out;
    Context ctx;
    ctx.pushKey("server");
    CHECK_FALSE(description.read(parsed.tree, out, ctx));

    SECTION("keys nest under the path already entered") {
        for (const cord::Diagnostic& diagnostic : ctx.diagnostics()) {
            INFO(diagnostic.message);
            CHECK(diagnostic.path.starts_with("server."));
        }
        CHECK(ctx.diagnostics().size() == 4);
    }
    SECTION("the path is left as it was found") {
        CHECK(ctx.path() == "server");
    }
    SECTION("every diagnostic explains itself") {
        for (const cord::Diagnostic& diagnostic : ctx.diagnostics()) {
            INFO(diagnostic.path);
            CHECK_FALSE(diagnostic.message.empty());
        }
    }
}
