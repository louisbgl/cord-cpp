// Every user-facing error and warning in one place, to read them and judge their wording.
// Not a test: nothing here is checked. Add a line whenever a new message appears in cord.
// Build and run: cmake --build build --parallel >/dev/null && ./build/tests/error_gallery

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "context.hpp"
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

void describe(cord::Struct<Server>& s) {
    s.add("name", &Server::name);
    s.add("port", &Server::port);
    s.add("ratio", &Server::ratio);
    s.add("verbose", &Server::verbose);
}

/** One member of type T under the key `value`, so converter cases show no absent-key noise. */
template <class T>
struct Single {
    T value{};
};

template <class T>
void describe(cord::Struct<Single<T>>& s) {
    s.add("value", &Single<T>::value);
}

void printDiagnostics(const std::vector<cord::Diagnostic>& diagnostics) {
    if (diagnostics.empty()) std::cout << "  (nothing reported)\n";
    for (const cord::Diagnostic& diagnostic : diagnostics) {
        std::cout << "  " << (diagnostic.severity == cord::Severity::Error ? "error" : "warning");
        if (diagnostic.line != 0) std::cout << " " << diagnostic.line << ":" << diagnostic.column;
        if (!diagnostic.path.empty()) std::cout << " [" << diagnostic.path << "]";
        std::cout << " " << diagnostic.message << "\n";
    }
}

void printSource(std::string_view source) {
    constexpr std::size_t max_shown = 60;
    if (source.size() <= max_shown) {
        std::cout << show(source) << "\n";
    } else {
        std::cout << show(source.substr(0, max_shown)) << "... (" << source.size() << " bytes)\n";
    }
}

/** Runs the whole pipeline the way parse<T> will: binding only happens if parsing found nothing. */
template <class T>
void bind(std::string_view source) {
    printSource(source);

    const cord::ParseResult parsed = cord::Parser(source).parse();
    if (!parsed.diagnostics.empty()) {
        printDiagnostics(parsed.diagnostics);
        return;
    }

    cord::Struct<T> description;
    describe(description);

    cord::Context ctx;
    T out{};
    description.read(parsed.tree, out, ctx);
    printDiagnostics(ctx.diagnostics());
}

void syntax(std::string_view source) { bind<Server>(source); }

template <class T>
void value(std::string_view literal) { bind<Single<T>>("value = " + std::string(literal)); }

void section(std::string_view title) { std::cout << "\n=== " << title << "\n"; }

} // namespace

int main() {
    section("Lexer: characters");
    syntax("@ = 1");
    syntax("a = @");
    syntax("db.host = \"x\"");

    section("Lexer: numbers");
    syntax("a = -");
    syntax("a = 1.");
    syntax("a = .5");
    syntax("a = 1e5");
    syntax("a = 0xFF");
    syntax("a = 123abc");

    section("Lexer: strings");
    syntax("a = \"oops");
    syntax("a = \"line one\nb = 1");
    syntax(R"(a = "bad \q")");

    section("Parser: entries");
    syntax("port 8080");
    syntax("fullscreen");
    syntax("a =");
    syntax("a =\nb = 1");
    syntax("= 1");
    syntax("1 = 2");
    syntax("\"a\" = 2");
    syntax("true = 1");
    syntax("a = 1  b = 2");
    syntax("name = bob");
    syntax("flag = True");
    syntax("a = = 1");
    syntax("]");
    syntax("a = ]");
    syntax("a = 1]");
    syntax("a = 1,");

    section("Parser: arrays");
    syntax("a = [1 2]");
    syntax("a = [,]");
    syntax("a = [1,,2]");
    syntax("a = [1}");
    syntax("a = [x = 1]");
    syntax("a = [");
    syntax("a = [1, 2");
    syntax("a = [[1, 2]");
    syntax("a = [1\nb = 2");

    section("Parser: objects");
    syntax("a = { x = 1 y = 2 }");
    syntax("a = { , }");
    syntax("a = { x }");
    syntax("a = { x = }");
    syntax("a = { 1 }");
    syntax("a = { x = 1 ]");
    syntax("a = {");
    syntax("a = { x = 1");

    section("Parser: nesting limit");
    syntax("a = " + std::string(257, '[') + std::string(257, ']'));

    section("Parser: several errors");
    syntax("a = @\nb = 1\nc = @");
    syntax("a = @\nb = @\nc = @");

    section("Converter: bool");
    value<bool>("1");
    value<bool>("\"true\"");

    section("Converter: integer");
    value<int>("\"8080\"");
    value<int>("8080.0");
    value<int>("true");
    value<int>("[]");
    value<int>("99999999999");
    value<int>("-99999999999");
    value<std::uint8_t>("256");
    value<unsigned>("-1");

    section("Converter: float");
    value<double>("\"1.5\"");
    value<double>("true");
    value<float>("1" + std::string(39, '0'));
    value<float>("1" + std::string(39, '0') + ".0");
    value<float>("0." + std::string(60, '0') + "1");

    section("Converter: string");
    value<std::string>("1");
    value<std::string>("{}");

    section("Struct: keys");
    bind<Server>("port = 1");
    bind<Server>("name = \"x\"\nport = 1\nratio = 1.5\nverbose = true\nextra = 2");
    bind<Server>("name = \"x\"\nport = 1\nport = 2\nratio = 1.5\nverbose = true");

    section("Struct: everything at once");
    bind<Server>("port = \"x\"\nport = true\nratio = \"y\"\nextra = 1");
}
