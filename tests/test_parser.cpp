#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <string>
#include <string_view>
#include <variant>

#include "overloaded.hpp"
#include "parser.hpp"
#include "test_helpers.hpp"

using cord::Array;
using cord::Bool;
using cord::Entry;
using cord::Float;
using cord::Int;
using cord::Node;
using cord::Object;
using cord::ParseResult;
using cord::Parser;
using cord::String;

namespace {

/**
 * @brief Prints text as a string literal, so a decoded newline stays on one line.
 */
std::string dump_string(std::string_view text) {
    std::string result = "\"";
    for (const char c : text) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\t': result += "\\t"; break;
            case '\r': result += "\\r"; break;
            default: result += c;
        }
    }
    return result + "\"";
}

std::string dump(const Node& node);

/**
 * @brief Prints entries as `key = value`, joined by the separator.
 */
std::string dump_entries(const Object& entries, std::string_view separator) {
    std::string result;
    for (std::size_t i = 0; i < entries.size(); i++) {
        if (i > 0) result += separator;
        result += entries[i].key + " = " + dump(entries[i].value);
    }
    return result;
}

/**
 * @brief Prints a node in a normalized form: `[1, 2]`, `{ a = 1, b = 2 }`, `[]`, `{}`.
 */
std::string dump(const Node& node) {
    return std::visit(cord::overloaded{
        [](const Bool& value) -> std::string { return value.value ? "true" : "false"; },
        [](const Int& value) -> std::string { return value.text; },
        [](const Float& value) -> std::string { return value.text; },
        [](const String& value) -> std::string { return dump_string(value.value); },
        [](const Array& elements) -> std::string {
            std::string result = "[";
            for (std::size_t i = 0; i < elements.size(); i++) {
                if (i > 0) result += ", ";
                result += dump(elements[i]);
            }
            return result + "]";
        },
        [](const Object& entries) -> std::string {
            if (entries.empty()) return "{}";
            return "{ " + dump_entries(entries, ", ") + " }";
        },
    }, node.value);
}

/**
 * @brief Prints a whole tree, one top-level entry per line.
 */
std::string dump(const Object& tree) {
    return dump_entries(tree, "\n");
}

/**
 * @brief Describes a source and what the parser made of it, printed with every failure.
 */
std::string trace(std::string_view source, const ParseResult& result) {
    std::string text = "input: " + show(source) + "\ntree:\n" + dump(result.tree) + "\ndiagnostics:";
    for (const cord::Diagnostic& diagnostic : result.diagnostics) {
        text += "\n  " + std::to_string(diagnostic.line) + ":" + std::to_string(diagnostic.column)
              + " " + diagnostic.message;
    }
    return text;
}

/**
 * @brief Prints where each diagnostic is, in order: `1:5 2:5`.
 */
std::string positions(const std::vector<cord::Diagnostic>& diagnostics) {
    std::string result;
    for (const cord::Diagnostic& diagnostic : diagnostics) {
        if (!result.empty()) result += ' ';
        result += std::to_string(diagnostic.line) + ":" + std::to_string(diagnostic.column);
    }
    return result;
}

} // namespace

// Macros, not functions: a failure must point at the test's line, not at a helper's.

/** The source parses with no diagnostics, into this printed tree. */
#define CHECK_TREE(source, expected)                                        \
    do {                                                                    \
        const ParseResult result = Parser(source).parse();                  \
        INFO(trace(source, result));                                        \
        CHECK(result.diagnostics.empty());                                  \
        CHECK(dump(result.tree) == (expected));                             \
    } while (false)

/** The source fails with exactly one syntax error, at this position. */
#define CHECK_ERROR_AT(source, expected_line, expected_column)              \
    do {                                                                    \
        const ParseResult result = Parser(source).parse();                  \
        INFO(trace(source, result));                                        \
        REQUIRE(result.diagnostics.size() == 1);                            \
        const cord::Diagnostic& diagnostic = result.diagnostics[0];         \
        CHECK(diagnostic.severity == cord::Severity::Error);                \
        CHECK(diagnostic.line == (expected_line));                          \
        CHECK(diagnostic.column == (expected_column));                      \
        CHECK(diagnostic.path.empty());                                     \
        CHECK_FALSE(diagnostic.message.empty());                            \
    } while (false)

/** The source reports errors at exactly these positions, and keeps this printed tree. */
#define CHECK_RECOVERY(source, expected_positions, expected_tree)           \
    do {                                                                    \
        const ParseResult result = Parser(source).parse();                  \
        INFO(trace(source, result));                                        \
        CHECK(positions(result.diagnostics) == (expected_positions));       \
        CHECK(dump(result.tree) == (expected_tree));                        \
    } while (false)

/** The first diagnostic of the source mentions this text. */
#define CHECK_MESSAGE_HAS(source, text)                                     \
    do {                                                                    \
        const ParseResult result = Parser(source).parse();                  \
        INFO(trace(source, result));                                        \
        REQUIRE_FALSE(result.diagnostics.empty());                          \
        CHECK(result.diagnostics[0].message.find(text) != std::string::npos); \
    } while (false)

/** The value of `a = <literal>` holds this alternative of the Node variant. */
#define CHECK_HOLDS(literal, Alternative)                                   \
    do {                                                                    \
        const std::string source = "a = " + std::string(literal);           \
        const ParseResult result = Parser(source).parse();                  \
        INFO(trace(source, result));                                        \
        REQUIRE(result.tree.size() == 1);                                   \
        CHECK(std::holds_alternative<Alternative>(result.tree[0].value.value)); \
    } while (false)

/** The string literal in `a = <literal>` decodes to exactly these bytes. */
#define CHECK_DECODED(literal, expected)                                    \
    do {                                                                    \
        const std::string source = "a = " + std::string(literal);           \
        const ParseResult result = Parser(source).parse();                  \
        INFO(trace(source, result));                                        \
        REQUIRE(result.tree.size() == 1);                                   \
        const auto* string = std::get_if<String>(&result.tree[0].value.value); \
        REQUIRE(string != nullptr);                                         \
        CHECK(string->value == std::string_view(expected));                 \
    } while (false)

TEST_CASE("Empty content", "[parser][content]") {
    auto source = GENERATE(as<std::string_view>{},
        "", "\n", "\r\n", "\n\n\n", "   ", "# comment", "# comment\n", "\n# one\n\n# two\n");
    CHECK_TREE(source, "");
}

TEST_CASE("Entries", "[parser][content]") {
    SECTION("one entry, with or without a final newline") {
        CHECK_TREE("a = 1", "a = 1");
        CHECK_TREE("a = 1\n", "a = 1");
        CHECK_TREE("a = 1\r\n", "a = 1");
    }
    SECTION("spacing does not matter") {
        CHECK_TREE("a=1", "a = 1");
        CHECK_TREE("  a   =\t1  ", "a = 1");
    }
    SECTION("several entries keep their order") {
        CHECK_TREE("b = 1\na = 2\nc = 3", "b = 1\na = 2\nc = 3");
    }
    SECTION("blank lines and comments between entries") {
        CHECK_TREE("\n# top\na = 1 # inline\n\n\n# middle\nb = 2\n\n", "a = 1\nb = 2");
    }
    SECTION("CRLF line endings") {
        CHECK_TREE("a = 1\r\nb = 2\r\n", "a = 1\nb = 2");
    }
    SECTION("duplicate keys are kept, binding warns about them") {
        CHECK_TREE("a = 1\na = 2", "a = 1\na = 2");
        CHECK_TREE("o = { a = 1, a = 2 }", "o = { a = 1, a = 2 }");
    }
    SECTION("keys are kept as written") {
        CHECK_TREE("kebab-case_Key1 = 1", "kebab-case_Key1 = 1");
    }
}

TEST_CASE("Scalars", "[parser][scalar]") {
    SECTION("each literal holds its own alternative") {
        CHECK_HOLDS("true", Bool);
        CHECK_HOLDS("false", Bool);
        CHECK_HOLDS("1", Int);
        CHECK_HOLDS("-1", Int);
        CHECK_HOLDS("1.0", Float);
        CHECK_HOLDS("-0.5", Float);
        CHECK_HOLDS(R"("1")", String);
        CHECK_HOLDS(R"("true")", String);
    }
    SECTION("bools hold their value") {
        CHECK_TREE("a = true\nb = false", "a = true\nb = false");
    }
    SECTION("numbers stay as written") {
        auto source = GENERATE(as<std::string_view>{},
            "007", "-0", "1.50", "-0.0", "99999999999999999999999", "3.14159265358979323846");
        const std::string line = "a = " + std::string(source);
        CHECK_TREE(line, line);
    }
}

TEST_CASE("String decoding", "[parser][scalar]") {
    SECTION("the quotes are removed") {
        CHECK_DECODED(R"("")", "");
        CHECK_DECODED(R"("hello world")", "hello world");
    }
    SECTION("every escape") {
        CHECK_DECODED(R"("\"")", "\"");
        CHECK_DECODED(R"("\\")", "\\");
        CHECK_DECODED(R"("\n")", "\n");
        CHECK_DECODED(R"("\t")", "\t");
        CHECK_DECODED(R"("\r")", "\r");
    }
    SECTION("escapes inside content") {
        CHECK_DECODED(R"("a\nb\tc")", "a\nb\tc");
        CHECK_DECODED(R"("say \"hi\"")", "say \"hi\"");
    }
    SECTION("an escaped backslash is not the start of another escape") {
        CHECK_DECODED(R"("\\n")", "\\n");
        CHECK_DECODED(R"("\\\n")", "\\\n");
    }
    SECTION("other bytes pass through") {
        CHECK_DECODED("\"caf\xC3\xA9\"", "caf\xC3\xA9");
        CHECK_DECODED(R"("# not a comment")", "# not a comment");
    }
}

TEST_CASE("Arrays", "[parser][array]") {
    SECTION("empty") {
        CHECK_TREE("a = []", "a = []");
        CHECK_TREE("a = [ ]", "a = []");
        CHECK_TREE("a = [\n]", "a = []");
    }
    SECTION("elements") {
        CHECK_TREE("a = [1]", "a = [1]");
        CHECK_TREE("a = [1, 2, 3]", "a = [1, 2, 3]");
        CHECK_TREE("a = [1,2,3]", "a = [1, 2, 3]");
    }
    SECTION("one trailing comma") {
        CHECK_TREE("a = [1,]", "a = [1]");
        CHECK_TREE("a = [1, 2, ]", "a = [1, 2]");
    }
    SECTION("mixed types, binding decides") {
        CHECK_TREE(R"(a = [1, "two", 3.0, true, [], {}])", R"(a = [1, "two", 3.0, true, [], {}])");
    }
    SECTION("nested") {
        CHECK_TREE("a = [[1, 2], [3, 4]]", "a = [[1, 2], [3, 4]]");
        CHECK_TREE("a = [[[]]]", "a = [[[]]]");
    }
    SECTION("over several lines") {
        CHECK_TREE("a = [\n  1,\n  2,\n]\nb = 3", "a = [1, 2]\nb = 3");
        CHECK_TREE("a = [\r\n  1,\r\n  2\r\n]\r\n", "a = [1, 2]");
        CHECK_TREE("a = [ # first\n  1, # one\n  # two\n  2\n]", "a = [1, 2]");
    }
}

TEST_CASE("Objects", "[parser][object]") {
    SECTION("empty") {
        CHECK_TREE("a = {}", "a = {}");
        CHECK_TREE("a = { }", "a = {}");
        CHECK_TREE("a = {\n}", "a = {}");
    }
    SECTION("entries keep their order") {
        CHECK_TREE("a = { y = 1 }", "a = { y = 1 }");
        CHECK_TREE("a = { y = 1, x = 2 }", "a = { y = 1, x = 2 }");
        CHECK_TREE("a = {y=1,x=2}", "a = { y = 1, x = 2 }");
    }
    SECTION("one trailing comma") {
        CHECK_TREE("a = { x = 1, }", "a = { x = 1 }");
    }
    SECTION("nested") {
        CHECK_TREE("a = { b = { c = { d = 1 } } }", "a = { b = { c = { d = 1 } } }");
        CHECK_TREE("a = { b = [1, { c = [] }] }", "a = { b = [1, { c = [] }] }");
        CHECK_TREE("a = [{ x = 1 }, { x = 2 },]", "a = [{ x = 1 }, { x = 2 }]");
    }
    SECTION("over several lines") {
        CHECK_TREE("a = {\n  x = 1,\n  y = 2,\n}\nb = 3", "a = { x = 1, y = 2 }\nb = 3");
    }
}

TEST_CASE("Node positions", "[parser][position]") {
    SECTION("a value starts at its first token") {
        const ParseResult result = Parser("port = 8080\n  name = \"x\"").parse();
        INFO(trace("port = 8080\n  name = \"x\"", result));
        REQUIRE(result.tree.size() == 2);
        CHECK(result.tree[0].value.line == 1);
        CHECK(result.tree[0].value.column == 8);
        CHECK(result.tree[1].value.line == 2);
        CHECK(result.tree[1].value.column == 10);
    }
    SECTION("an array starts at its bracket, its elements at their own position") {
        const ParseResult result = Parser("a = [\n  1,\n  2\n]").parse();
        INFO(trace("a = [\n  1,\n  2\n]", result));
        REQUIRE(result.tree.size() == 1);
        const Node& array = result.tree[0].value;
        CHECK(array.line == 1);
        CHECK(array.column == 5);
        const auto* elements = std::get_if<Array>(&array.value);
        REQUIRE(elements != nullptr);
        REQUIRE(elements->size() == 2);
        CHECK((*elements)[1].line == 3);
        CHECK((*elements)[1].column == 3);
    }
    SECTION("an object starts at its brace, its values at their own position") {
        const ParseResult result = Parser("a = { x = 1 }").parse();
        INFO(trace("a = { x = 1 }", result));
        REQUIRE(result.tree.size() == 1);
        const Node& object = result.tree[0].value;
        CHECK(object.column == 5);
        const auto* entries = std::get_if<Object>(&object.value);
        REQUIRE(entries != nullptr);
        REQUIRE(entries->size() == 1);
        CHECK((*entries)[0].value.column == 11);
    }
}

TEST_CASE("Rejected entries", "[parser][error]") {
    SECTION("missing equals") {
        CHECK_ERROR_AT("port 8080", 1, 6);
        CHECK_ERROR_AT("fullscreen", 1, 11);
        CHECK_ERROR_AT("fullscreen\nb = 1", 1, 11);
    }
    SECTION("missing value") {
        CHECK_ERROR_AT("a =", 1, 4);
        CHECK_ERROR_AT("a = \nb = 1", 1, 5);
    }
    SECTION("missing key") {
        CHECK_ERROR_AT("= 1", 1, 1);
        CHECK_ERROR_AT("1 = 2", 1, 1);
        CHECK_ERROR_AT("\"a\" = 2", 1, 1);
    }
    SECTION("reserved word as a key") {
        CHECK_ERROR_AT("true = 1", 1, 1);
    }
    SECTION("two entries on one line") {
        CHECK_ERROR_AT("a = 1  b = 2", 1, 8);
    }
    SECTION("a key is not a value") {
        CHECK_ERROR_AT("name = bob", 1, 8);
        CHECK_ERROR_AT("flag = True", 1, 8);
    }
    SECTION("two equals") {
        CHECK_ERROR_AT("a = = 1", 1, 5);
    }
    SECTION("stray closers and commas") {
        CHECK_ERROR_AT("]", 1, 1);
        CHECK_ERROR_AT("a = ]", 1, 5);
        CHECK_ERROR_AT("a = 1]", 1, 6);
        CHECK_ERROR_AT("a = 1,", 1, 6);
    }
}

TEST_CASE("Rejected arrays", "[parser][error]") {
    SECTION("missing comma") {
        CHECK_ERROR_AT("list = [1 2]", 1, 11);
        CHECK_ERROR_AT("a = [1\n2]", 2, 1);
    }
    SECTION("extra commas") {
        CHECK_ERROR_AT("a = [,]", 1, 6);
        CHECK_ERROR_AT("a = [1,,2]", 1, 8);
        CHECK_ERROR_AT("a = [1,,]", 1, 8);
    }
    SECTION("wrong closer") {
        CHECK_ERROR_AT("a = [1}", 1, 7);
        CHECK_ERROR_AT("a = [}", 1, 6);
    }
    SECTION("an entry is not an element") {
        CHECK_ERROR_AT("a = [x = 1]", 1, 6);
    }
}

TEST_CASE("Rejected objects", "[parser][error]") {
    SECTION("missing comma") {
        CHECK_ERROR_AT("a = { x = 1 y = 2 }", 1, 13);
    }
    SECTION("extra commas") {
        CHECK_ERROR_AT("a = { , }", 1, 7);
        CHECK_ERROR_AT("a = { x = 1,, }", 1, 13);
    }
    SECTION("incomplete entries") {
        CHECK_ERROR_AT("a = { x }", 1, 9);
        CHECK_ERROR_AT("a = { x = }", 1, 11);
    }
    SECTION("a value is not an entry") {
        CHECK_ERROR_AT("a = { 1 }", 1, 7);
        CHECK_ERROR_AT("a = { true = 1 }", 1, 7);
    }
    SECTION("wrong closer") {
        CHECK_ERROR_AT("a = { x = 1 ]", 1, 13);
        CHECK_ERROR_AT("a = { ]", 1, 7);
    }
}

TEST_CASE("Unclosed brackets are reported at their opening", "[parser][error]") {
    SECTION("arrays") {
        CHECK_ERROR_AT("list = [1, 2", 1, 8);
        CHECK_ERROR_AT("a = [", 1, 5);
        CHECK_ERROR_AT("a = [1,", 1, 5);
    }
    SECTION("objects") {
        CHECK_ERROR_AT("a = {", 1, 5);
        CHECK_ERROR_AT("a = { x = 1", 1, 5);
        CHECK_ERROR_AT("a = { x = 1,", 1, 5);
    }
    SECTION("the innermost open one") {
        CHECK_ERROR_AT("a = [[1, 2", 1, 6);
        CHECK_ERROR_AT("a = [[1], [2", 1, 11);
        CHECK_ERROR_AT("a = { b = [1", 1, 11);
    }
    SECTION("the outer one, when the inner one closed") {
        CHECK_ERROR_AT("a = [[1, 2]", 1, 5);
    }
    SECTION("on an earlier line, with trailing newlines swallowed") {
        CHECK_ERROR_AT("x = 1\na = [\n  1,\n\n", 2, 5);
    }
}

TEST_CASE("Lexer errors become diagnostics", "[parser][error]") {
    SECTION("reported where the bad token starts") {
        CHECK_ERROR_AT("x = .5", 1, 5);
        CHECK_ERROR_AT("x = 1e5", 1, 5);
        CHECK_ERROR_AT("x = 0xFF", 1, 5);
        CHECK_ERROR_AT("n = 123abc", 1, 5);
        CHECK_ERROR_AT(R"(s = "bad \q")", 1, 5);
        CHECK_ERROR_AT(R"(s = "unterminated)", 1, 5);
        CHECK_ERROR_AT("db.host = \"x\"", 1, 3);
    }
    SECTION("anywhere in the structure") {
        CHECK_ERROR_AT("a = [1, @]", 1, 9);
        CHECK_ERROR_AT("a = { x = @ }", 1, 11);
        CHECK_ERROR_AT("@ = 1", 1, 1);
    }
    SECTION("the message names an unexpected character") {
        const ParseResult result = Parser("a = @").parse();
        INFO(trace("a = @", result));
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics[0].message.find('@') != std::string::npos);
    }
}

TEST_CASE("Error messages", "[parser][error]") {
    SECTION("they name the right closer") {
        CHECK_MESSAGE_HAS("a = [1 2]", "']'");
        CHECK_MESSAGE_HAS("a = { x = 1 y = 2 }", "'}'");
        CHECK_MESSAGE_HAS("a = [", "']'");
        CHECK_MESSAGE_HAS("a = {", "'}'");
    }
    SECTION("they name the offending token") {
        CHECK_MESSAGE_HAS("port 8080", "'8080'");
        CHECK_MESSAGE_HAS("= 1", "'='");
    }
    SECTION("end of file and end of line are spelled out") {
        CHECK_MESSAGE_HAS("a =", "end of file");
        CHECK_MESSAGE_HAS("a =\nb = 1", "end of line");
        const ParseResult result = Parser("a =\r\nb = 1").parse();
        INFO(trace("a =\r\nb = 1", result));
        REQUIRE_FALSE(result.diagnostics.empty());
        CHECK(result.diagnostics[0].message.find_first_of("\r\n") == std::string::npos);
    }
    SECTION("lexer errors keep their own message and the bad text") {
        CHECK_MESSAGE_HAS("x = 1e5", "Invalid number");
        CHECK_MESSAGE_HAS("x = 1e5", "1e5");
        CHECK_MESSAGE_HAS(R"(s = "bad \q")", "Unknown escape sequence");
        CHECK_MESSAGE_HAS(R"(s = "oops)", "Unterminated string");
        CHECK_MESSAGE_HAS("@ = 1", "'@'");
    }
}

TEST_CASE("Nesting limit", "[parser][error][nesting]") {
    constexpr std::size_t limit = 256;
    const auto arrays = [](std::size_t depth) {
        return "a = " + std::string(depth, '[') + std::string(depth, ']');
    };
    const auto objects = [](std::size_t depth) {
        std::string source = "a = ";
        for (std::size_t i = 0; i < depth; i++) source += "{ a = ";
        source += "1";
        for (std::size_t i = 0; i < depth; i++) source += " }";
        return source;
    };

    SECTION("the limit itself is fine") {
        CHECK_TREE(arrays(limit), arrays(limit));
        CHECK_TREE(objects(limit), objects(limit));
    }
    SECTION("one over is reported at the bracket that goes over") {
        CHECK_ERROR_AT(arrays(limit + 1), 1, 5 + limit);
        CHECK_ERROR_AT(objects(limit + 1), 1, 5 + limit * 6);
        CHECK_MESSAGE_HAS(arrays(limit + 1), "256");
    }
    SECTION("arrays and objects count together") {
        std::string source = "a = ";
        for (std::size_t i = 0; i < limit; i++) source += "[{ a = ";
        CHECK_ERROR_AT(source, 1, 5 + (limit / 2) * 7);
    }
    SECTION("far past the limit does not crash") {
        CHECK_ERROR_AT(arrays(1'000'000), 1, 5 + limit);
        CHECK_ERROR_AT("a = " + std::string(1'000'000, '['), 1, 5 + limit);
    }
    SECTION("parsing resumes on the next line") {
        CHECK_RECOVERY(arrays(limit + 1) + "\nb = 1", "1:261", "b = 1");
    }
    SECTION("depth is back to zero for the next entry") {
        CHECK_TREE(arrays(limit) + "\n" + arrays(limit), arrays(limit) + "\n" + arrays(limit));
    }
}

TEST_CASE("Recovering at the next top-level line", "[parser][error][recovery]") {
    SECTION("parsing resumes after the bad line, which is not in the tree") {
        CHECK_RECOVERY("a = @\nb = 2", "1:5", "b = 2");
        CHECK_RECOVERY("a = @\r\nb = 2", "1:5", "b = 2");
        CHECK_RECOVERY("a = 1\n@\nb = 2", "2:1", "a = 1\nb = 2");
        CHECK_RECOVERY("a b\nc = 1", "1:3", "c = 1");
        CHECK_RECOVERY("a = 1\nb = [1, @]\nc = 3", "2:9", "a = 1\nc = 3");
    }
    SECTION("the rest of a bad line is skipped without more errors") {
        CHECK_RECOVERY("a = @ @ @\nb = 2", "1:5", "b = 2");
        CHECK_RECOVERY("= = =\nb = 2", "1:1", "b = 2");
    }
    SECTION("a valid entry stays when garbage follows it on its line") {
        CHECK_RECOVERY("a = 1 b = 2\nc = 3", "1:7", "a = 1\nc = 3");
        CHECK_RECOVERY("a = [1] @ @\nc = 3", "1:9", "a = [1]\nc = 3");
    }
    SECTION("a bad line at the end") {
        CHECK_RECOVERY("a = 1\nb = @", "2:5", "a = 1");
        CHECK_RECOVERY("a = 1\nb = @\n", "2:5", "a = 1");
    }
    SECTION("newlines inside closed brackets do not end the skip") {
        CHECK_RECOVERY("a = [\n  1, @,\n  2,\n]\nb = 3", "2:6", "b = 3");
        CHECK_RECOVERY("a = {\n  x = @,\n  y = 2,\n}\nb = 3", "2:7", "b = 3");
        CHECK_RECOVERY("a = [[@], [2]]\nb = 3", "1:7", "b = 3");
    }
    SECTION("stray and wrong closers do not hide the next line") {
        CHECK_RECOVERY("]\nb = 1", "1:1", "b = 1");
        CHECK_RECOVERY("a = ]\nb = 1", "1:5", "b = 1");
        CHECK_RECOVERY("a = [1}\nb = 1", "1:7", "b = 1");
    }
    SECTION("an unterminated string only costs its own line") {
        CHECK_RECOVERY("a = \"oops\nb = 1", "1:5", "b = 1");
    }
}

TEST_CASE("Stopping after two bad entries in a row", "[parser][error][recovery]") {
    SECTION("the second error is reported, then nothing else is parsed") {
        CHECK_RECOVERY("a = @\nb = @\nc = @", "1:5 2:5", "");
        CHECK_RECOVERY("a = @\nb = @\nc = 1", "1:5 2:5", "");
        CHECK_RECOVERY("a b\nc d\ne = 1", "1:3 2:3", "");
        CHECK_RECOVERY("a = \n1", "1:5 2:1", "");
    }
    SECTION("the tree keeps the entries before them") {
        CHECK_RECOVERY("a = 1\nb = @\nc = @\nd = 2", "2:5 3:5", "a = 1");
    }
    SECTION("blank and comment lines do not separate them") {
        CHECK_RECOVERY("a = @\n\n# comment\nb = @\nc = 1", "1:5 4:5", "");
    }
    SECTION("a good entry in between resets the count") {
        CHECK_RECOVERY("a = @\nb = 1\nc = @\nd = 2\ne = @", "1:5 3:5 5:5", "b = 1\nd = 2");
        CHECK_RECOVERY("a = @\nb = 1\nc = @\nd = @\ne = 2", "1:5 3:5 4:5", "b = 1");
    }
}

TEST_CASE("Unclosed brackets swallow the rest", "[parser][error][recovery]") {
    SECTION("one error at the opening, entries before it are kept") {
        CHECK_RECOVERY("x = 1\na = [1,\n2,\n", "2:5", "x = 1");
    }
    SECTION("a bad element inside one reports nothing further") {
        CHECK_RECOVERY("a = [@\nb = 2\nc = 3", "1:6", "");
    }
}

TEST_CASE("The grammar's example config", "[parser][integration]") {
    constexpr std::string_view source =
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
        "]\n";

    constexpr std::string_view expected =
        "name = \"cord\"\n"
        "port = 8080\n"
        "ratio = -0.75\n"
        "verbose = false\n"
        "empty = []\n"
        "tags = [\"fast\", \"small\"]\n"
        "grid = [[1, 2], [3, 4]]\n"
        "player = { name = \"bob\", pos = { x = 1.5, y = -2.0 } }\n"
        "entities = [{ name = \"a\", hp = 10 }, { name = \"b\", hp = 20 }]";

    CHECK_TREE(source, expected);
}
