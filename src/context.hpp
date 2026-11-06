#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "diagnostic.hpp"
#include "node.hpp"

namespace cord {

/**
 * @brief The state of one bind: the problems found so far, and the path being read.
 *
 * Every diagnostic takes the current path, e.g. `entities[0].pos.x`.
 * A problem with no token to point at, like an absent key, is at position 0:0.
 */
class Context {
public:
    /** @brief Reports an error at this node's position. */
    void error(const Node& node, std::string message) {
        _push(Severity::Error, node.line, node.column, std::move(message));
    }

    /** @brief Reports an error with no position. */
    void error(std::string message) {
        _push(Severity::Error, 0, 0, std::move(message));
    }

    /** @brief Reports a warning at this node's position. */
    void warning(const Node& node, std::string message) {
        _push(Severity::Warning, node.line, node.column, std::move(message));
    }

    /** @brief Reports a warning with no position. */
    void warning(std::string message) {
        _push(Severity::Warning, 0, 0, std::move(message));
    }

    /** @brief Enters a key: `pos` then `x` gives `pos.x`. Undo it with popPath(). */
    void pushKey(std::string_view key) {
        _marks.push_back(_path.size());
        if (!_path.empty()) _path += '.';
        _path += key;
    }

    /** @brief Enters an array element: `tags` then 2 gives `tags[2]`. Undo it with popPath(). */
    void pushIndex(std::size_t index) {
        _marks.push_back(_path.size());
        _path += '[' + std::to_string(index) + ']';
    }

    /** @brief Leaves the last key or index entered. */
    void popPath() {
        _path.resize(_marks.back());
        _marks.pop_back();
    }

    const std::string& path() const { return _path; }
    const std::vector<Diagnostic>& diagnostics() const { return _diagnostics; }

private:
    std::vector<Diagnostic> _diagnostics;
    std::string _path;
    std::vector<std::size_t> _marks;

    void _push(Severity severity, std::size_t line, std::size_t column, std::string message) {
        _diagnostics.push_back(Diagnostic {
            .severity = severity,
            .line = line,
            .column = column,
            .path = _path,
            .message = std::move(message),
        });
    }
};

} // namespace cord
