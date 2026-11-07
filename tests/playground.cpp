// Manual playground, not a test: nothing here is checked, and it does not run with the test suite.
// Build and run: cmake --build build --parallel >/dev/null && ./build/tests/playground [file]
// Prints the tokens, tree and diagnostics of the file, or of `source` below without one.
// std::cout debug written inside cord shows up here directly.

#include <concepts>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <optional>

#include "context.hpp"
#include "converter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "struct.hpp"
#include "test_helpers.hpp"

namespace {

struct Server {
    std::string name = "default";
    int port = 8080;
    double ratio = 0.5;
    bool verbose = false;
};

struct Meta {
    Server server{};
    bool something = true;
};

struct TestVec {
    std::vector<bool> booleans = {true, false, true, true};
};

struct TestOptional {
    std::optional<std::string> opt = std::nullopt;
};

void describe(cord::Struct<Server>& s) {
    s.add("name", &Server::name);
    s.add("port", &Server::port);
    s.add("ratio", &Server::ratio);
    s.add("verbose", &Server::verbose);
}

void describe(cord::Struct<Meta>& s) {
    s.add("server", &Meta::server);
    s.add("something", &Meta::something);
}

void describe(cord::Struct<TestVec>& s) {
    s.add("booleans", &TestVec::booleans);
}

void describe(cord::Struct<TestOptional>& s) {
    s.add("opt", &TestOptional::opt);
}

void printValue(const Server& server) {
    std::cout << "  name=" << server.name << " port=" << server.port << " ratio=" << server.ratio
              << " verbose=" << std::boolalpha << server.verbose << "\n";
}

void printValue(const Meta& meta) {
    std::cout << "Meta:\n";
    printValue(meta.server);
    std::cout << "  something=" << std::boolalpha << meta.something << "\n";
}

void printValue(const TestVec& vec) {
    std::cout << "  booleans = [" << std::boolalpha;
    for (std::size_t i = 0; i < vec.booleans.size(); i++) {
        if (i > 0) std::cout << ", ";
        std::cout << vec.booleans[i];
    }
    std::cout << "] (" << vec.booleans.size() << " elements)\n";
}

void printValue(const TestOptional& opt) {
    if (opt.opt)
        std::cout << "TestOptional: '" << opt.opt.value() << "'\n";
    else
        std::cout << "TestOptional: nullopt";
}

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
 * @brief Runs Struct<T>::read on a whole config and prints the result. T needs a printValue().
 */
template <class T>
void tryStruct(std::string_view text) {
    std::cout << show(text) << "\n";

    const cord::ParseResult parsed = cord::Parser(text).parse();
    if (!parsed.diagnostics.empty()) {
        std::cout << "  did not parse\n";
        printDiagnostics(parsed.diagnostics);
        return;
    }

    cord::Struct<T> description;
    describe(description);

    cord::Context ctx;
    T out{};
    const bool ok = description.read(parsed.tree, out, ctx);
    std::cout << "  read returned " << std::boolalpha << ok << "\n";
    printValue(out);
    printDiagnostics(ctx.diagnostics());
}

/**
 * @brief Free space: whatever is being tried right now goes here.
 */
void scratch() {
    tryStruct<Server>("name = \"cord\"\nport = 1\nratio = 2.5\nverbose = true");
    tryStruct<Server>("port = 1");
    tryStruct<Server>("port = 1\nextra = 2");
    tryStruct<Server>("port = 1\nport = 2");
    tryStruct<Server>("port = \"not an int\"\nname = \"still read\"");
    std::cout << "\n================\n\n";
    
    tryStruct<Meta>("server = { name = \"cord\", port = 1, ratio = 2.5, verbose = false }\nsomething = false");
    std::cout << "\n================\n\n";
    
    tryStruct<TestVec>("booleans = [false, false, true, false]");
    tryStruct<TestVec>("booleans = [false, false, true, false, \"not a boolean\", true]");
    tryStruct<TestVec>("booleans = []");
    tryStruct<TestVec>("# empty source");
    std::cout << "\n================\n\n";

    tryStruct<TestOptional>("opt = \"this sting is optional\"");
    tryStruct<TestOptional>("# empty source");
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
