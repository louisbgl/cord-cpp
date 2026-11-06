#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "context.hpp"
#include "parser.hpp"
#include "struct.hpp"
#include "test_helpers.hpp"

using cord::Context;
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
    STATIC_CHECK_FALSE(CanAdd<Pair, char>);
    STATIC_CHECK_FALSE(CanAdd<Pair, std::map<std::string, int>>);
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
