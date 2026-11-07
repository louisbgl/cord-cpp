#pragma once

#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "context.hpp"
#include "converter.hpp"
#include "diagnostic.hpp"
#include "parser.hpp"
#include "struct.hpp"

namespace cord {

/**
 * @brief What parse<T> made of a config: the value if it is valid, and everything worth reporting.
 */
template <class T>
struct Result {
    std::optional<T> value;               ///< Empty if any error was found.
    std::vector<Diagnostic> diagnostics;  ///< Errors and warnings, in the order they were found.

    /** @brief Whether the config is invalid. Then value is empty. */
    bool hasErrors() const {
        for (const auto& d : diagnostics) {
            if (d.severity == Severity::Error) return true;
        }
        return false;
    }

    /** @brief Whether something is worth a look, even though the value may be usable. */
    bool hasWarnings() const {
        for (const auto& d : diagnostics) {
            if (d.severity == Severity::Warning) return true;
        }
        return false;
    }

    /**
     * @brief Copies the value into out.
     * @return false if there is no value, leaving out untouched.
     */
    bool get(T& out) const {
        if (!value.has_value()) return false;
        out = *value;
        return true;
    }
};

/**
 * @brief Parses a config into a T, which needs a `describe(cord::Struct<T>&)`.
 *
 * With a syntax error, binding is skipped and only syntax errors are reported (docs/grammar.md, section 3).
 */
template <class T>
Result<T> parse(std::string_view source) {
    // A plain bool, or compilers follow the message with a long explanation of the concept.
    constexpr bool describable = Describable<T>;
    static_assert(describable, CORD_COMPILE_ERROR(
        "cord::parse<T> needs a struct with a describe(cord::Struct<T>&).\n"
        "Declare it before calling parse."));

    // Keeps a bad type from adding more errors after the one above.
    if constexpr (describable) {
        ParseResult parsed = Parser(source).parse();
        Result<T> result {
            .value = std::nullopt,
            .diagnostics = std::move(parsed.diagnostics),
        };
        if (!result.diagnostics.empty()) return result;

        Context ctx;
        T out{};
        const bool ok = description<T>().read(parsed.tree, out, ctx);
        result.diagnostics = ctx.diagnostics();
        if (!ok) return result;

        result.value = std::move(out);
        return result;
    }
}

} // namespace cord
