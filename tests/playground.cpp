// Manual playground, not a test: nothing here is checked, and it does not run with the test suite.
// Build and run: cmake --build build --parallel >/dev/null && ./build/tests/playground [file]
// Prints the tokens, tree and diagnostics of the file, or of `source` below without one.
// std::cout debug written inside cord shows up here directly.

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "context.hpp"
#include "converter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "test_helpers.hpp"

namespace {

constexpr std::string_view source = R"(name = "cord"
port = 8080
tags = ["fast", "small"]
player = { name = "bob", pos = { x = 1.5, y = -2.0 } }
)";

void printDiagnostics(const std::vector<cord::Diagnostic>& diagnostics) {
    for (const cord::Diagnostic& diagnostic : diagnostics) {
        std::cout << "  " << diagnostic.line << ":" << diagnostic.column;
        if (!diagnostic.path.empty()) std::cout << " [" << diagnostic.path << "]";
        std::cout << " " << diagnostic.message << "\n";
    }
}

void printTokens(std::string_view text) {
    constexpr std::size_t max_tokens = 10000; // a next() that does not advance must not hang this

    std::cout << "--- tokens\n";
    cord::Lexer lexer(text);
    for (std::size_t i = 0; i < max_tokens; i++) {
        const cord::Token token = lexer.next();
        std::cout << "  " << token.line << ":" << token.column << " " << tokenName(token.type) << " " << show(token.lexeme);
        if (!token.message.empty()) std::cout << " (" << token.message << ")";
        std::cout << "\n";
        if (token.type == cord::TokenType::Eof) return;
    }
    std::cout << "  stopped after " << max_tokens << " tokens\n";
}

void printTree(std::string_view text) {
    const cord::ParseResult result = cord::Parser(text).parse();
    std::cout << "\n--- tree\n" << dump(result.tree) << "\n\n--- parser diagnostics\n";
    printDiagnostics(result.diagnostics);
}

/**
 * @brief Runs Converter<T> on a value, as written in a config, and prints what came out.
 */
template <class T>
void tryRead(std::string_view literal) {
    std::cout << show(literal) << " -> ";

    const std::string text = "a = " + std::string(literal);
    const cord::ParseResult parsed = cord::Parser(text).parse();
    if (parsed.tree.empty()) {
        std::cout << "did not parse\n";
        printDiagnostics(parsed.diagnostics);
        return;
    }

    cord::Context ctx;
    T out{};
    if (cord::Converter<T>::read(parsed.tree[0].value, out, ctx)) {
        // The unary + makes int8_t and uint8_t print as numbers, not as characters.
        if constexpr (std::same_as<T, bool>) std::cout << std::boolalpha << out << "\n";
        else if constexpr (std::integral<T>) std::cout << +out << "\n";
        else std::cout << out << "\n";
    } else {
        std::cout << "refused\n";
    }
    printDiagnostics(ctx.diagnostics());
}

/**
 * @brief Free space: whatever is being tried right now goes here.
 */
void scratch() {
    tryRead<bool>("true");
    tryRead<std::string>(R"("hello\nworld")");
    tryRead<bool>("1");
    tryRead<int>("8080");
    tryRead<std::uint8_t>("256");
    tryRead<double>("1");
}

} // namespace

int main(int argc, char** argv) {
    std::string text{source};
    if (argc > 1) {
        std::ifstream file(argv[1]);
        if (!file) {
            std::cerr << "Could not open " << argv[1] << "\n";
            return 1;
        }
        std::stringstream content;
        content << file.rdbuf();
        text = content.str();
    }

    printTokens(text);
    printTree(text);
    std::cout << "\n--- scratch\n";
    scratch();
}
