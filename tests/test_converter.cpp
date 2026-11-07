#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "context.hpp"
#include "converter.hpp"
#include "parser.hpp"
#include "test_helpers.hpp"

using cord::Context;
using cord::Converter;
using cord::Node;
using cord::Parser;

namespace {

/**
 * @brief Parses `a = <literal>` and returns the value, so tests get real nodes with real positions.
 */
Node parseValue(std::string_view literal) {
    const std::string source = "a = " + std::string(literal);
    cord::ParseResult result = Parser(source).parse();
    INFO(show(source));
    REQUIRE(result.diagnostics.empty());
    REQUIRE(result.tree.size() == 1);
    return std::move(result.tree[0].value);
}

/**
 * @brief Prints the diagnostics for a failure message.
 */
std::string trace(std::string_view literal, const Context& ctx) {
    std::string text = "literal: " + show(literal) + "\ndiagnostics:";
    for (const cord::Diagnostic& diagnostic : ctx.diagnostics()) {
        text += "\n  " + std::to_string(diagnostic.line) + ":" + std::to_string(diagnostic.column)
              + " [" + diagnostic.path + "] " + diagnostic.message;
    }
    return text;
}

template <class T>
std::string maxOf() { return std::to_string(std::numeric_limits<T>::max()); }

template <class T>
std::string minOf() { return std::to_string(std::numeric_limits<T>::min()); }

/** A decimal integer literal with this many digits, far past what small types can hold. */
std::string digits(std::size_t count) { return "1" + std::string(count - 1, '0'); }

} // namespace

// Macros, not functions: a failure must point at the test's line, not at a helper's.

/** The literal converts to this value, with no diagnostics. */
#define CHECK_READS(T, literal, expected)                                   \
    do {                                                                    \
        const Node node = parseValue(literal);                              \
        Context ctx;                                                        \
        T out{};                                                            \
        const bool ok = Converter<T>::read(node, out, ctx);                 \
        INFO(trace(literal, ctx));                                          \
        CHECK(ok);                                                          \
        CHECK(ctx.diagnostics().empty());                                   \
        CHECK(out == (expected));                                           \
    } while (false)

/** The literal is refused with exactly one error, at the value's position. */
#define CHECK_REJECTS(T, literal)                                           \
    do {                                                                    \
        const Node node = parseValue(literal);                              \
        Context ctx;                                                        \
        T out{};                                                            \
        const bool ok = Converter<T>::read(node, out, ctx);                 \
        INFO(trace(literal, ctx));                                          \
        CHECK_FALSE(ok);                                                    \
        REQUIRE(ctx.diagnostics().size() == 1);                             \
        const cord::Diagnostic& diagnostic = ctx.diagnostics()[0];          \
        CHECK(diagnostic.severity == cord::Severity::Error);                \
        CHECK(diagnostic.line == 1);                                        \
        CHECK(diagnostic.column == 5);                                      \
        CHECK_FALSE(diagnostic.message.empty());                            \
    } while (false)

TEST_CASE("Context paths", "[context]") {
    Context ctx;
    CHECK(ctx.path().empty());

    SECTION("keys join with a dot") {
        ctx.pushKey("player");
        ctx.pushKey("pos");
        CHECK(ctx.path() == "player.pos");
    }
    SECTION("indexes attach to their key") {
        ctx.pushKey("grid");
        ctx.pushIndex(1);
        ctx.pushIndex(0);
        CHECK(ctx.path() == "grid[1][0]");
    }
    SECTION("a key after an index") {
        ctx.pushKey("entities");
        ctx.pushIndex(0);
        ctx.pushKey("pos");
        ctx.pushKey("x");
        CHECK(ctx.path() == "entities[0].pos.x");
    }
    SECTION("popping undoes exactly one step") {
        ctx.pushKey("entities");
        ctx.pushIndex(12);
        ctx.pushKey("hp");
        ctx.popPath();
        CHECK(ctx.path() == "entities[12]");
        ctx.popPath();
        CHECK(ctx.path() == "entities");
        ctx.popPath();
        CHECK(ctx.path().empty());
    }
}

TEST_CASE("Context diagnostics", "[context]") {
    Context ctx;
    const Node node = parseValue("1");

    SECTION("errors and warnings keep their severity and order") {
        ctx.error(node, "first");
        ctx.warning(node, "second");
        REQUIRE(ctx.diagnostics().size() == 2);
        CHECK(ctx.diagnostics()[0].severity == cord::Severity::Error);
        CHECK(ctx.diagnostics()[0].message == "first");
        CHECK(ctx.diagnostics()[1].severity == cord::Severity::Warning);
        CHECK(ctx.diagnostics()[1].message == "second");
    }
    SECTION("with a node, the position is the node's") {
        ctx.error(node, "bad");
        REQUIRE(ctx.diagnostics().size() == 1);
        CHECK(ctx.diagnostics()[0].line == 1);
        CHECK(ctx.diagnostics()[0].column == 5);
    }
    SECTION("without a node, the position is 0:0") {
        ctx.error("absent");
        ctx.warning("absent too");
        REQUIRE(ctx.diagnostics().size() == 2);
        CHECK(ctx.diagnostics()[0].line == 0);
        CHECK(ctx.diagnostics()[0].column == 0);
        CHECK(ctx.diagnostics()[1].severity == cord::Severity::Warning);
        CHECK(ctx.diagnostics()[1].line == 0);
    }
    SECTION("a diagnostic keeps the path from when it was reported") {
        ctx.pushKey("port");
        ctx.error(node, "bad");
        ctx.popPath();
        ctx.error(node, "bad too");
        REQUIRE(ctx.diagnostics().size() == 2);
        CHECK(ctx.diagnostics()[0].path == "port");
        CHECK(ctx.diagnostics()[1].path.empty());
    }
}

TEST_CASE("Which types can be converted", "[converter]") {
    STATIC_CHECK(cord::Convertible<bool>);
    STATIC_CHECK(cord::Convertible<int>);
    STATIC_CHECK(cord::Convertible<unsigned long long>);
    STATIC_CHECK(cord::Convertible<std::int8_t>);
    STATIC_CHECK(cord::Convertible<std::uint8_t>);
    STATIC_CHECK(cord::Convertible<float>);
    STATIC_CHECK(cord::Convertible<double>);
    STATIC_CHECK(cord::Convertible<std::string>);

    STATIC_CHECK_FALSE(cord::Convertible<char>);
    STATIC_CHECK_FALSE(cord::Convertible<wchar_t>);
    STATIC_CHECK_FALSE(cord::Convertible<char8_t>);
    STATIC_CHECK_FALSE(cord::Convertible<char16_t>);
    STATIC_CHECK_FALSE(cord::Convertible<char32_t>);
    STATIC_CHECK_FALSE(cord::Convertible<const char*>);
    STATIC_CHECK_FALSE(cord::Convertible<std::map<std::string, int>>);
}

TEST_CASE("Bools", "[converter][bool]") {
    SECTION("accepted") {
        CHECK_READS(bool, "true", true);
        CHECK_READS(bool, "false", false);
    }
    SECTION("other types are refused") {
        CHECK_REJECTS(bool, "1");
        CHECK_REJECTS(bool, "0");
        CHECK_REJECTS(bool, "1.0");
        CHECK_REJECTS(bool, R"("true")");
        CHECK_REJECTS(bool, "[]");
        CHECK_REJECTS(bool, "{}");
    }
}

TEST_CASE("Integers", "[converter][int]") {
    SECTION("accepted") {
        CHECK_READS(int, "0", 0);
        CHECK_READS(int, "8080", 8080);
        CHECK_READS(int, "-1", -1);
        CHECK_READS(int, "-0", 0);
    }
    SECTION("leading zeros are decimal, not octal") {
        CHECK_READS(int, "007", 7);
        CHECK_READS(int, "010", 10);
    }
    SECTION("the limits of each type fit") {
        CHECK_READS(int, maxOf<int>(), std::numeric_limits<int>::max());
        CHECK_READS(int, minOf<int>(), std::numeric_limits<int>::min());
        CHECK_READS(std::int8_t, "127", std::int8_t{127});
        CHECK_READS(std::int8_t, "-128", std::int8_t{-128});
        CHECK_READS(std::uint8_t, "255", std::uint8_t{255});
        CHECK_READS(std::int16_t, "-32768", std::int16_t{-32768});
        CHECK_READS(std::uint32_t, maxOf<std::uint32_t>(), std::numeric_limits<std::uint32_t>::max());
        CHECK_READS(std::int64_t, maxOf<std::int64_t>(), std::numeric_limits<std::int64_t>::max());
        CHECK_READS(std::int64_t, minOf<std::int64_t>(), std::numeric_limits<std::int64_t>::min());
        CHECK_READS(std::uint64_t, maxOf<std::uint64_t>(), std::numeric_limits<std::uint64_t>::max());
    }
    SECTION("one past the limits is refused") {
        CHECK_REJECTS(int, "2147483648");
        CHECK_REJECTS(int, "-2147483649");
        CHECK_REJECTS(std::int8_t, "128");
        CHECK_REJECTS(std::int8_t, "-129");
        CHECK_REJECTS(std::uint8_t, "256");
        CHECK_REJECTS(std::uint64_t, "18446744073709551616");
    }
    SECTION("far past the limits is refused") {
        CHECK_REJECTS(int, "99999999999");
        CHECK_REJECTS(std::int64_t, "99999999999999999999999");
    }
    SECTION("a negative number does not fit an unsigned type") {
        CHECK_REJECTS(unsigned, "-1");
        CHECK_REJECTS(std::uint8_t, "-1");
        CHECK_REJECTS(std::uint64_t, "-1");
    }
    SECTION("other types are refused") {
        CHECK_REJECTS(int, "8080.0");
        CHECK_REJECTS(int, "1.5");
        CHECK_REJECTS(int, R"("8080")");
        CHECK_REJECTS(int, "true");
        CHECK_REJECTS(int, "[]");
        CHECK_REJECTS(int, "{}");
    }
}

TEST_CASE("Floats", "[converter][float]") {
    SECTION("accepted") {
        CHECK_READS(double, "1.5", 1.5);
        CHECK_READS(double, "-0.75", -0.75);
        CHECK_READS(double, "0.0", 0.0);
        CHECK_READS(float, "1.5", 1.5f);
        CHECK_READS(float, "-0.75", -0.75f);
    }
    SECTION("the closest value of the type is picked") {
        CHECK_READS(double, "0.1", 0.1);
        CHECK_READS(float, "0.1", 0.1f);
        CHECK_READS(double, "3.14159265358979323846", 3.14159265358979323846);
    }
    SECTION("an int literal is accepted") {
        CHECK_READS(double, "1", 1.0);
        CHECK_READS(double, "-3", -3.0);
        CHECK_READS(float, "0", 0.0f);
        CHECK_READS(float, "42", 42.0f);
    }
    SECTION("precision loss from a large int is silent") {
        CHECK_READS(double, "9007199254740993", 9007199254740992.0);
        CHECK_READS(float, "16777217", 16777216.0f);
    }
    SECTION("too large for the type is refused") {
        CHECK_REJECTS(float, digits(40));
        CHECK_REJECTS(float, digits(40) + ".0");
        CHECK_REJECTS(float, "-" + digits(40));
        CHECK_REJECTS(double, digits(400));
        CHECK_REJECTS(double, digits(400) + ".0");
    }
    SECTION("a value too large for float can still fit double") {
        CHECK_READS(double, digits(40), 1e39);
    }
    SECTION("too small for the type is refused, not rounded to zero") {
        CHECK_REJECTS(float, "0." + std::string(60, '0') + "1");
        CHECK_REJECTS(float, "-0." + std::string(60, '0') + "1");
        CHECK_REJECTS(double, "0." + std::string(400, '0') + "1");
    }
    SECTION("a value too small for float can still fit double") {
        CHECK_READS(double, "0." + std::string(60, '0') + "1", 1e-61);
    }
    SECTION("other types are refused") {
        CHECK_REJECTS(double, R"("1.5")");
        CHECK_REJECTS(double, "true");
        CHECK_REJECTS(double, "[]");
        CHECK_REJECTS(double, "{}");
    }
}

TEST_CASE("Strings", "[converter][string]") {
    SECTION("accepted") {
        CHECK_READS(std::string, R"("")", "");
        CHECK_READS(std::string, R"("hello world")", "hello world");
    }
    SECTION("the value is the decoded one") {
        CHECK_READS(std::string, R"("a\nb")", "a\nb");
        CHECK_READS(std::string, R"("say \"hi\"")", "say \"hi\"");
    }
    SECTION("other types are refused") {
        CHECK_REJECTS(std::string, "1");
        CHECK_REJECTS(std::string, "1.5");
        CHECK_REJECTS(std::string, "true");
        CHECK_REJECTS(std::string, "[]");
        CHECK_REJECTS(std::string, "{}");
    }
}

TEST_CASE("Errors carry the current path", "[converter][path]") {
    const Node node = parseValue("true");
    Context ctx;
    ctx.pushKey("server");
    ctx.pushKey("port");
    int out = 0;
    CHECK_FALSE(Converter<int>::read(node, out, ctx));
    REQUIRE(ctx.diagnostics().size() == 1);
    CHECK(ctx.diagnostics()[0].path == "server.port");
}

/**
 * @brief Each diagnostic as `1:9 [1]`, in the order they were reported.
 */
std::vector<std::string> errorsOf(const Context& ctx) {
    std::vector<std::string> lines;
    for (const cord::Diagnostic& diagnostic : ctx.diagnostics()) {
        lines.push_back(std::to_string(diagnostic.line) + ":" + std::to_string(diagnostic.column) + " " + diagnostic.path);
    }
    return lines;
}

/** The literal is refused, with exactly these errors (`line:column path`), in this order. */
#define CHECK_ERRORS(T, literal, ...)                                       \
    do {                                                                    \
        const Node node = parseValue(literal);                              \
        Context ctx;                                                        \
        T out{};                                                            \
        const bool ok = Converter<T>::read(node, out, ctx);                 \
        INFO(trace(literal, ctx));                                          \
        CHECK_FALSE(ok);                                                    \
        CHECK(errorsOf(ctx) == std::vector<std::string>{__VA_ARGS__});      \
        for (const cord::Diagnostic& diagnostic : ctx.diagnostics())        \
            CHECK(diagnostic.severity == cord::Severity::Error);            \
    } while (false)

TEST_CASE("Which vectors can be converted", "[converter][vector]") {
    STATIC_CHECK(cord::Convertible<std::vector<int>>);
    STATIC_CHECK(cord::Convertible<std::vector<bool>>);
    STATIC_CHECK(cord::Convertible<std::vector<double>>);
    STATIC_CHECK(cord::Convertible<std::vector<std::string>>);
    STATIC_CHECK(cord::Convertible<std::vector<std::vector<int>>>);

    STATIC_CHECK_FALSE(cord::Convertible<std::vector<char>>);
    STATIC_CHECK_FALSE(cord::Convertible<std::vector<const char*>>);
    STATIC_CHECK_FALSE(cord::Convertible<std::vector<std::map<std::string, int>>>);
}

TEST_CASE("Vectors", "[converter][vector]") {
    SECTION("accepted") {
        CHECK_READS(std::vector<int>, "[1, 2, 3]", (std::vector<int>{1, 2, 3}));
        CHECK_READS(std::vector<std::string>, R"(["a", "b\nc"])", (std::vector<std::string>{"a", "b\nc"}));
        CHECK_READS(std::vector<double>, "[1.5, -0.75]", (std::vector<double>{1.5, -0.75}));
    }
    SECTION("empty") {
        CHECK_READS(std::vector<int>, "[]", std::vector<int>{});
    }
    SECTION("bools, which std::vector stores as bits") {
        CHECK_READS(std::vector<bool>, "[true, false, true]", (std::vector<bool>{true, false, true}));
    }
    SECTION("each element goes through its own converter") {
        CHECK_READS(std::vector<double>, "[1, 2.5]", (std::vector<double>{1.0, 2.5}));
    }
    SECTION("a trailing comma is not an element") {
        CHECK_READS(std::vector<int>, "[1, 2,]", (std::vector<int>{1, 2}));
    }
    SECTION("nested") {
        CHECK_READS(std::vector<std::vector<int>>, "[[1, 2], [], [3]]",
            (std::vector<std::vector<int>>{{1, 2}, {}, {3}}));
    }
    SECTION("over several lines") {
        CHECK_READS(std::vector<int>, "[\n  1,\n  2,\n]", (std::vector<int>{1, 2}));
    }
}

TEST_CASE("A vector replaces what was there", "[converter][vector]") {
    const auto readInto = [](std::vector<int> out, std::string_view literal) {
        const Node node = parseValue(literal);
        Context ctx;
        REQUIRE(Converter<std::vector<int>>::read(node, out, ctx));
        return out;
    };

    SECTION("a shorter array does not keep the extra old elements") {
        CHECK(readInto({9, 9, 9, 9}, "[1, 2]") == std::vector<int>{1, 2});
    }
    SECTION("a longer array does not keep the old ones in front") {
        CHECK(readInto({9}, "[1, 2, 3]") == std::vector<int>{1, 2, 3});
    }
    SECTION("an empty array empties it") {
        CHECK(readInto({9, 9}, "[]").empty());
    }
}

TEST_CASE("Rejected vectors", "[converter][vector]") {
    SECTION("anything but an array is refused at the value") {
        CHECK_REJECTS(std::vector<int>, "1");
        CHECK_REJECTS(std::vector<int>, R"("[1, 2]")");
        CHECK_REJECTS(std::vector<int>, "{}");
        CHECK_REJECTS(std::vector<int>, "{ a = 1 }");
    }
    SECTION("a bad element is reported at itself, with its index") {
        CHECK_ERRORS(std::vector<int>, R"([1, "x", 3])", "1:9 [1]");
        CHECK_ERRORS(std::vector<std::uint8_t>, "[1, 256]", "1:9 [1]");
        CHECK_ERRORS(std::vector<bool>, "[true, 1]", "1:12 [1]");
    }
    SECTION("each bad element is its own error") {
        CHECK_ERRORS(std::vector<int>, R"(["x", 2, "y"])", "1:6 [0]", "1:14 [2]");
        CHECK_ERRORS(std::vector<int>, "[true, false, true]", "1:6 [0]", "1:12 [1]", "1:19 [2]");
    }
    SECTION("nested indexes stack up") {
        CHECK_ERRORS(std::vector<std::vector<int>>, R"([[1], [2, "x"]])", "1:15 [1][1]");
        CHECK_ERRORS(std::vector<std::vector<int>>, "[[1], 2]", "1:11 [1]");
    }
}

TEST_CASE("Vector paths", "[converter][vector][path]") {
    const Node node = parseValue(R"([1, "x"])");
    Context ctx;
    ctx.pushKey("tags");
    std::vector<int> out;
    CHECK_FALSE(Converter<std::vector<int>>::read(node, out, ctx));

    SECTION("the index follows the key") {
        REQUIRE(ctx.diagnostics().size() == 1);
        CHECK(ctx.diagnostics()[0].path == "tags[1]");
    }
    SECTION("the path is left as it was found") {
        CHECK(ctx.path() == "tags");
    }
}
