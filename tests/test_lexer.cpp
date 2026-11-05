#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/catch_tostring.hpp>

#include <string>
#include <string_view>
#include <vector>

#include "lexer.hpp"
#include "test_helpers.hpp"

using cord::Lexer;
using cord::Token;
using cord::TokenType;
using enum cord::TokenType;

template <>
struct Catch::StringMaker<TokenType> {
    static std::string convert(TokenType type) {
        switch (type) {
            case Key: return "Key";
            case Bool: return "Bool";
            case Int: return "Int";
            case Float: return "Float";
            case String: return "String";
            case Equals: return "Equals";
            case Comma: return "Comma";
            case LBracket: return "LBracket";
            case RBracket: return "RBracket";
            case LBrace: return "LBrace";
            case RBrace: return "RBrace";
            case Newline: return "Newline";
            case Eof: return "Eof";
            case Error: return "Error";
        }
        return "?";
    }
};

namespace {

/**
 * @brief Lexes up to and including Eof, or the first Error when stop_at_error is set.
 *
 * Most tests stop at Error, so they stay independent of how the lexer resumes after one.
 * The cap turns a next() that does not advance into a failed test instead of eating all memory.
 */
std::vector<Token> lex(std::string_view source, bool stop_at_error = true) {
    constexpr std::size_t max_tokens = 100;

    Lexer lexer(source);
    std::vector<Token> tokens;
    do {
        tokens.push_back(lexer.next());
    } while (tokens.back().type != Eof && !(stop_at_error && tokens.back().type == Error)
             && tokens.size() < max_tokens);
    return tokens;
}

/**
 * @brief Token types of lex(source, stop_at_error), the final Eof or Error included.
 */
std::vector<TokenType> types(std::string_view source, bool stop_at_error = true) {
    std::vector<TokenType> result;
    for (const Token& token : lex(source, stop_at_error)) result.push_back(token.type);
    return result;
}

/**
 * @brief Describes a source and what the lexer made of it, printed with every failure.
 */
std::string trace(std::string_view source, bool stop_at_error = true) {
    std::string result = "input: " + show(source) + "\nlexer returned:";
    for (const Token& token : lex(source, stop_at_error)) {
        result += "\n  " + Catch::StringMaker<TokenType>::convert(token.type) + " " + show(token.lexeme)
                + " at " + std::to_string(token.line) + ":" + std::to_string(token.column);
        if (!token.message.empty()) result += " (" + std::string(token.message) + ")";
    }
    return result;
}

} // namespace

// Macros, not functions: a failure must point at the test's line, not at a helper's.

#define CHECK_TYPES(source, ...)                                            \
    do {                                                                    \
        INFO(trace(source));                                                \
        CHECK(types(source) == std::vector<TokenType>{__VA_ARGS__});        \
    } while (false)

/** Same as CHECK_TYPES, but keeps lexing after an Error. */
#define CHECK_TYPES_THROUGH_ERRORS(source, ...)                             \
    do {                                                                    \
        INFO(trace(source, false));                                         \
        CHECK(types(source, false) == std::vector<TokenType>{__VA_ARGS__}); \
    } while (false)

/** The whole source is one token of this type, followed by Eof. */
#define CHECK_SINGLE(source, expected_type)                                 \
    do {                                                                    \
        INFO(trace(source));                                                \
        const auto tokens = lex(source);                                    \
        REQUIRE(tokens.size() == 2);                                        \
        CHECK(tokens[0].type == (expected_type));                           \
        CHECK(tokens[0].lexeme == (source));                                \
        CHECK(tokens[1].type == Eof);                                       \
    } while (false)

/** Lexing the source reaches an Error token that carries a message. */
#define CHECK_ERROR(source)                                                 \
    do {                                                                    \
        INFO(trace(source));                                                \
        const auto tokens = lex(source);                                    \
        CHECK(tokens.back().type == Error);                                 \
        CHECK_FALSE(tokens.back().message.empty());                         \
    } while (false)

#define CHECK_LEXEME(source, index, expected_lexeme)                        \
    do {                                                                    \
        INFO(trace(source));                                                \
        const auto tokens = lex(source);                                    \
        REQUIRE(tokens.size() > (index));                                   \
        CHECK(tokens[index].lexeme == (expected_lexeme));                   \
    } while (false)

#define CHECK_POSITION(source, index, expected_line, expected_column)       \
    do {                                                                    \
        INFO(trace(source));                                                \
        const auto tokens = lex(source);                                    \
        REQUIRE(tokens.size() > (index));                                   \
        CHECK(tokens[index].line == (expected_line));                       \
        CHECK(tokens[index].column == (expected_column));                   \
    } while (false)

TEST_CASE("Eof", "[lexer][eof]") {
    SECTION("empty source") {
        CHECK_TYPES("", Eof);
    }
    SECTION("only whitespace") {
        CHECK_TYPES("  \t  ", Eof);
    }
    SECTION("only a comment") {
        CHECK_TYPES("# nothing here", Eof);
    }
    SECTION("has an empty lexeme") {
        CHECK_LEXEME("a", 1, "");
    }
    SECTION("repeats once reached") {
        Lexer lexer("a");
        lexer.next();
        CHECK(lexer.next().type == Eof);
        CHECK(lexer.next().type == Eof);
        CHECK(lexer.next().type == Eof);
    }
}

TEST_CASE("Key", "[lexer][key]") {
    SECTION("valid keys") {
        auto source = GENERATE(as<std::string_view>{},
            "a", "port", "PORT", "camelCase", "snake_case", "kebab-case",
            "_", "_private", "a1", "a-1", "a_1", "trailing-", "trailing_");
        CHECK_SINGLE(source, Key);
    }
    SECTION("case-sensitive reserved words are plain keys") {
        auto source = GENERATE(as<std::string_view>{}, "True", "TRUE", "False", "FALSE");
        CHECK_SINGLE(source, Key);
    }
    SECTION("longest match wins over reserved words") {
        auto source = GENERATE(as<std::string_view>{},
            "truex", "true1", "true_", "true-x", "falsey", "tru", "fals");
        CHECK_SINGLE(source, Key);
    }
    SECTION("cannot start with a dash") {
        CHECK_ERROR("-abc");
    }
    SECTION("no dots") {
        CHECK_LEXEME("db.host", 0, "db");
        CHECK_ERROR("db.host");
    }
    SECTION("no non-ASCII") {
        CHECK_ERROR("é");
    }
}

TEST_CASE("Bool", "[lexer][bool]") {
    auto source = GENERATE(as<std::string_view>{}, "true", "false");
    CHECK_SINGLE(source, Bool);
}

TEST_CASE("Int", "[lexer][int]") {
    SECTION("valid ints") {
        auto source = GENERATE(as<std::string_view>{},
            "0", "7", "8080", "-1", "-0", "007", "99999999999999999999999");
        CHECK_SINGLE(source, Int);
    }
    SECTION("a dash directly after an int starts a new token") {
        CHECK_TYPES("1-2", Int, Int, Eof);
    }
    SECTION("rejected forms") {
        auto source = GENERATE(as<std::string_view>{},
            "123abc", "1_000", "0xFF", "1e5", "-", "--1", "- 1", "+1");
        CHECK_ERROR(source);
    }
}

TEST_CASE("Float", "[lexer][float]") {
    SECTION("valid floats") {
        auto source = GENERATE(as<std::string_view>{},
            "0.0", "1.5", "-0.75", "3.14159", "1.50", "007.5", "-0.0");
        CHECK_SINGLE(source, Float);
    }
    SECTION("rejected forms") {
        auto source = GENERATE(as<std::string_view>{},
            ".5", "-.5", "5.", "-5.", "1.2.3", "1.5x", "1.5_", "1.5e3", ".");
        CHECK_ERROR(source);
    }
}

TEST_CASE("String", "[lexer][string]") {
    SECTION("plain content") {
        auto source = GENERATE(as<std::string_view>{},
            R"("")", R"("hello")", R"("two words")", "\"tab\there\"", R"("not # a comment")",
            R"("= , [ ] { }")", R"("true")", R"("123")", "\"caf\xC3\xA9\"");
        CHECK_SINGLE(source, String);
    }
    SECTION("every valid escape, kept raw") {
        auto source = GENERATE(as<std::string_view>{},
            R"("\"")", R"("\\")", R"("\n")", R"("\t")", R"("\r")", R"("a\"b\\c\nd\te\rf")");
        CHECK_SINGLE(source, String);
    }
    SECTION("an escaped backslash does not escape the closing quote") {
        CHECK_TYPES(R"("a\\" b)", String, Key, Eof);
        CHECK_LEXEME(R"("a\\" b)", 0, R"("a\\")");
    }
    SECTION("unknown escapes") {
        auto source = GENERATE(as<std::string_view>{}, R"("bad \q")", R"("\0")", R"("\x41")", R"("\ ")");
        CHECK_ERROR(source);
    }
    SECTION("unterminated") {
        auto source = GENERATE(as<std::string_view>{},
            R"(")", R"("abc)", R"("abc\")", R"("abc\)", "\"abc\n\"", "\"abc\r\n\"");
        CHECK_ERROR(source);
    }
    SECTION("single quotes are not strings") {
        CHECK_ERROR("'abc'");
    }
}

TEST_CASE("Punctuation", "[lexer][punctuation]") {
    CHECK_SINGLE("=", Equals);
    CHECK_SINGLE(",", Comma);
    CHECK_SINGLE("[", LBracket);
    CHECK_SINGLE("]", RBracket);
    CHECK_SINGLE("{", LBrace);
    CHECK_SINGLE("}", RBrace);
}

TEST_CASE("Newline", "[lexer][newline]") {
    SECTION("LF") {
        CHECK_SINGLE("\n", Newline);
    }
    SECTION("CRLF is a single token") {
        CHECK_SINGLE("\r\n", Newline);
    }
    SECTION("blank lines each produce a token") {
        CHECK_TYPES("\n\n", Newline, Newline, Eof);
    }
    SECTION("a lone CR is invalid") {
        CHECK_ERROR("\r");
        CHECK_ERROR("a\rb");
    }
}

TEST_CASE("Whitespace", "[lexer][whitespace]") {
    SECTION("is skipped around tokens") {
        CHECK_TYPES(" \t a \t ", Key, Eof);
        CHECK_LEXEME(" \t a \t ", 0, "a");
    }
    SECTION("is optional between tokens") {
        CHECK_TYPES("a=1", Key, Equals, Int, Eof);
        CHECK_TYPES("[1,2]", LBracket, Int, Comma, Int, RBracket, Eof);
    }
}

TEST_CASE("Comments", "[lexer][comment]") {
    SECTION("after a token") {
        CHECK_TYPES("a # comment", Key, Eof);
    }
    SECTION("stop before the newline") {
        CHECK_TYPES("# comment\na", Newline, Key, Eof);
    }
    SECTION("do not swallow the CR of a CRLF") {
        CHECK_TYPES("# comment\r\n", Newline, Eof);
        CHECK_LEXEME("# comment\r\n", 0, "\r\n");
    }
    SECTION("can hold anything") {
        CHECK_TYPES(R"(# "quote = [ { \q @)", Eof);
    }
    SECTION("directly after a token") {
        CHECK_TYPES("1#comment", Int, Eof);
    }
}

TEST_CASE("Newlines inside brackets", "[lexer][depth]") {
    SECTION("dropped in arrays") {
        CHECK_TYPES("[\n]", LBracket, RBracket, Eof);
    }
    SECTION("dropped in objects") {
        CHECK_TYPES("{\n}", LBrace, RBrace, Eof);
    }
    SECTION("CRLF dropped too") {
        CHECK_TYPES("[\r\n]", LBracket, RBracket, Eof);
    }
    SECTION("dropped at every depth") {
        CHECK_TYPES("[{\n[\n]\n}\n]", LBracket, LBrace, LBracket, RBracket, RBrace, RBracket, Eof);
    }
    SECTION("after a comment") {
        CHECK_TYPES("[ # comment\n]", LBracket, RBracket, Eof);
    }
    SECTION("kept once every bracket is closed") {
        CHECK_TYPES("[[]]\n", LBracket, LBracket, RBracket, RBracket, Newline, Eof);
    }
    SECTION("kept while depth is still zero after a stray closer") {
        CHECK_TYPES("]\n", RBracket, Newline, Eof);
        CHECK_TYPES("}\n", RBrace, Newline, Eof);
    }
    SECTION("a stray closer does not cancel a later opener") {
        CHECK_TYPES("][\n]\n", RBracket, LBracket, RBracket, Newline, Eof);
    }
}

TEST_CASE("Positions", "[lexer][position]") {
    SECTION("first token starts at 1:1") {
        CHECK_POSITION("a", 0, 1, 1);
    }
    SECTION("column advances by token length and whitespace") {
        CHECK_POSITION("port = 8080", 1, 1, 6);
        CHECK_POSITION("port = 8080", 2, 1, 8);
    }
    SECTION("a tab counts as one column") {
        CHECK_POSITION("\ta", 0, 1, 2);
    }
    SECTION("column counts bytes, not characters") {
        CHECK_POSITION("\"\xC3\xA9\" a", 1, 1, 6);
    }
    SECTION("LF moves to the next line, column 1") {
        CHECK_POSITION("a\nb", 1, 1, 2);
        CHECK_POSITION("a\nb", 2, 2, 1);
    }
    SECTION("CRLF moves one line only") {
        CHECK_POSITION("a\r\nb", 2, 2, 1);
    }
    SECTION("comments do not break the line count") {
        CHECK_POSITION("# one\n# two\na", 2, 3, 1);
    }
    SECTION("dropped newlines still count") {
        CHECK_POSITION("[\n\n  1\r\n]", 1, 3, 3);
        CHECK_POSITION("[\n\n  1\r\n]", 2, 4, 1);
    }
    SECTION("an escaped newline in a string is not a line break") {
        CHECK_POSITION(R"("a\nb" c)", 1, 1, 8);
    }
    SECTION("Eof sits right after the last token") {
        CHECK_POSITION("abc", 1, 1, 4);
    }
    SECTION("Eof after a final newline is on the next line") {
        CHECK_POSITION("a\n", 2, 2, 1);
    }
    SECTION("an invalid character is reported where it is") {
        CHECK_ERROR("port = @");
        CHECK_POSITION("port = @", 2, 1, 8);
    }
    SECTION("an unterminated string is reported at its opening quote") {
        CHECK_ERROR("a = \"abc");
        CHECK_POSITION("a = \"abc", 2, 1, 5);
    }
}

TEST_CASE("Valid tokens that only the parser can reject", "[lexer][shape]") {
    SECTION("missing equals") {
        CHECK_TYPES("port 8080", Key, Int, Eof);
    }
    SECTION("two entries on one line") {
        CHECK_TYPES("a = 1  b = 2", Key, Equals, Int, Key, Equals, Int, Eof);
    }
    SECTION("unquoted string") {
        CHECK_TYPES("name = bob", Key, Equals, Key, Eof);
    }
    SECTION("reserved word as a key") {
        CHECK_TYPES("true = 1", Bool, Equals, Int, Eof);
    }
    SECTION("unclosed bracket") {
        CHECK_TYPES("[1, 2", LBracket, Int, Comma, Int, Eof);
    }
}

TEST_CASE("A full config", "[lexer][integration]") {
    constexpr std::string_view source =
        "# server\n"
        "port = 8080\r\n"
        "tags = [\"fast\",\n"
        "        \"small\",]\n"
        "player = { name = \"bob\", pos = { x = -1.5 } } # inline\n"
        "on = true";

    CHECK_TYPES(source,
        Newline,
        Key, Equals, Int, Newline,
        Key, Equals, LBracket, String, Comma, String, Comma, RBracket, Newline,
        Key, Equals, LBrace,
        Key, Equals, String, Comma,
        Key, Equals, LBrace,
        Key, Equals, Float,
        RBrace, RBrace, Newline,
        Key, Equals, Bool,
        Eof);
}

TEST_CASE("Resuming after an Error", "[lexer][error]") {
    SECTION("every Error moves forward") {
        CHECK_TYPES_THROUGH_ERRORS("@@", Error, Error, Eof);
        CHECK_TYPES_THROUGH_ERRORS("a @ b", Key, Error, Key, Eof);
    }
    SECTION("the newline after an Error is still emitted, for the parser's recovery") {
        CHECK_TYPES_THROUGH_ERRORS("123abc\nb", Error, Newline, Key, Eof);
        CHECK_TYPES_THROUGH_ERRORS("\"abc\nb", Error, Newline, Key, Eof);
        CHECK_TYPES_THROUGH_ERRORS("\"abc\r\nb", Error, Newline, Key, Eof);
    }
    SECTION("the content of a bad string is never lexed as tokens") {
        CHECK_TYPES_THROUGH_ERRORS(R"("\q [ x" b)", Error, Key, Eof);
        CHECK_TYPES_THROUGH_ERRORS("\"abc [\nb", Error, Newline, Key, Eof);
    }
    SECTION("an Error does not change the bracket depth") {
        CHECK_TYPES_THROUGH_ERRORS("[\"\\q\"\n]\n", LBracket, Error, RBracket, Newline, Eof);
    }
    SECTION("position is still right afterwards") {
        Lexer lexer("\"\\q\"\n  a");
        lexer.next();
        lexer.next();
        const Token token = lexer.next();
        CHECK(token.type == Key);
        CHECK(token.line == 2);
        CHECK(token.column == 3);
    }
}

TEST_CASE("What an Error covers", "[lexer][error]") {
    SECTION("a whole malformed number") {
        auto source = GENERATE(as<std::string_view>{}, "123abc", "1.2.3", "0xFF", "1e5", "1_000", "1.5x", "-1.5.2");
        CHECK_ERROR(source);
        CHECK_LEXEME(source, 0, source);
    }
    SECTION("a number cut short") {
        CHECK_LEXEME("-", 0, "-");
        CHECK_LEXEME("- 1", 0, "-");
        CHECK_LEXEME("5.", 0, "5.");
    }
    SECTION("a whole string with an unknown escape") {
        CHECK_LEXEME(R"("bad \q here" b)", 0, R"("bad \q here")");
    }
    SECTION("an unterminated string, up to the end of the line") {
        CHECK_LEXEME(R"("abc)", 0, R"("abc)");
        CHECK_LEXEME(R"("abc\)", 0, R"("abc\)");
        CHECK_LEXEME("\"abc\nb", 0, "\"abc");
        CHECK_LEXEME("\"abc\r\nb", 0, "\"abc");
    }
    SECTION("one invalid character") {
        CHECK_LEXEME("@abc", 0, "@");
    }
}

TEST_CASE("Error positions", "[lexer][error][position]") {
    SECTION("an unknown escape is reported at the opening quote") {
        CHECK_POSITION(R"(a = "\q")", 2, 1, 5);
    }
    SECTION("on a later line") {
        CHECK_POSITION("a\n  @", 2, 2, 3);
    }
    SECTION("a string inside brackets, after dropped newlines") {
        CHECK_POSITION("[\n  \"\\q\"\n]", 1, 2, 3);
    }
}

TEST_CASE("Tokens with no separator", "[lexer][adjacent]") {
    SECTION("a string followed by anything, left to the parser") {
        CHECK_TYPES(R"("a"b)", String, Key, Eof);
        CHECK_TYPES(R"("a""b")", String, String, Eof);
        CHECK_TYPES(R"("a"1)", String, Int, Eof);
    }
    SECTION("reserved words") {
        CHECK_SINGLE("truefalse", Key);
        CHECK_TYPES("true=false", Bool, Equals, Bool, Eof);
        CHECK_TYPES("[true,false]", LBracket, Bool, Comma, Bool, RBracket, Eof);
    }
    SECTION("comments") {
        CHECK_TYPES(R"("a"#comment)", String, Eof);
        CHECK_TYPES("]#comment", RBracket, Eof);
    }
}

TEST_CASE("Unbalanced brackets", "[lexer][depth]") {
    SECTION("mismatched closers still count, the parser rejects them") {
        CHECK_TYPES("[}\n", LBracket, RBrace, Newline, Eof);
        CHECK_TYPES("{]\n", LBrace, RBracket, Newline, Eof);
    }
    SECTION("an unclosed bracket swallows later newlines") {
        CHECK_TYPES("[\n\na\n", LBracket, Key, Eof);
    }
    SECTION("the end of the source inside brackets still reaches Eof") {
        CHECK_TYPES("[", LBracket, Eof);
        CHECK_TYPES("[ ", LBracket, Eof);
        CHECK_TYPES("[ # comment", LBracket, Eof);
        CHECK_TYPES("{[\n", LBrace, LBracket, Eof);
    }
    SECTION("comments ending in CRLF") {
        CHECK_TYPES("[ # comment\r\n]", LBracket, RBracket, Eof);
    }
}

TEST_CASE("Odd bytes", "[lexer][bytes]") {
    using namespace std::string_view_literals;

    SECTION("a NUL byte does not end the source") {
        CHECK_SINGLE("\"a\0b\""sv, String);
        CHECK_TYPES("# a\0b\nc"sv, Newline, Key, Eof);
        CHECK_TYPES_THROUGH_ERRORS("a\0b"sv, Key, Error, Key, Eof);
    }
    SECTION("a lone CR is plain content in strings and comments") {
        CHECK_SINGLE("\"a\rb\"", String);
        CHECK_TYPES("# a\rb", Eof);
    }
    SECTION("a lone CR at the end of the source is invalid") {
        CHECK_TYPES_THROUGH_ERRORS("a\r", Key, Error, Eof);
    }
    SECTION("whitespace before a CRLF") {
        CHECK_TYPES("a \t\r\nb", Key, Newline, Key, Eof);
    }
    SECTION("a UTF-8 BOM at the very start is skipped") {
        CHECK_TYPES("\xEF\xBB\xBF" "a", Key, Eof);
        CHECK_LEXEME("\xEF\xBB\xBF" "a", 0, "a");
        CHECK_POSITION("\xEF\xBB\xBF" "a", 0, 1, 1);
        CHECK_TYPES("\xEF\xBB\xBF", Eof);
    }
    SECTION("a UTF-8 BOM anywhere else is invalid") {
        CHECK_ERROR("a \xEF\xBB\xBF");
        CHECK_ERROR(" \xEF\xBB\xBF" "a");
    }
}

TEST_CASE("LF and CRLF lex the same", "[lexer][integration]") {
    const std::string lf =
        "# comments run to end of line\n"
        "name    = \"cord\"            # string\n"
        "ratio   = -0.75\n"
        "verbose = false\n"
        "empty   = []\n"
        "\n"
        "tags = [\"fast\", \"small\",]\n"
        "grid = [[1, 2], [3, 4]]\n"
        "player = { name = \"bob\", pos = { x = 1.5, y = -2.0 } }\n"
        "entities = [\n"
        "    { name = \"a\", hp = 10 },\n"
        "    { name = \"b\", hp = 20 },\n"
        "]\n";

    std::string crlf;
    for (const char c : lf) crlf += c == '\n' ? std::string("\r\n") : std::string(1, c);

    const auto lf_tokens = lex(lf, false);
    const auto crlf_tokens = lex(crlf, false);
    INFO(trace(lf, false));

    REQUIRE(lf_tokens.size() == crlf_tokens.size());
    CHECK(lf_tokens.back().type == Eof);
    for (std::size_t i = 0; i < lf_tokens.size(); i++) {
        CAPTURE(i);
        CHECK(lf_tokens[i].type != Error);
        CHECK(lf_tokens[i].type == crlf_tokens[i].type);
        CHECK(lf_tokens[i].line == crlf_tokens[i].line);
        CHECK(lf_tokens[i].column == crlf_tokens[i].column);
        if (lf_tokens[i].type != Newline) CHECK(lf_tokens[i].lexeme == crlf_tokens[i].lexeme);
    }
}
