#pragma once

#include <exception>
#include <string>

namespace cord {

/**
 * @brief Exception class for errors in the cord library.
 */
class CordException : public std::exception {
public:
    explicit CordException(const std::string& message)
        : _message("[CORD] " + message) {}

    explicit CordException(const std::string& file, int line, const std::string& message)
        : _message("[CORD] " + file + ":" + std::to_string(line) + ": " + message) {}

    const char* what() const noexcept override {
        return _message.c_str();
    }

private:
    std::string _message;
};

} // namespace cord
