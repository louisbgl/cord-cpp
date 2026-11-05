#pragma once

#include <cstdio>
#include <string>
#include <string_view>

/**
 * @brief Wraps text in backticks with control and non-ASCII bytes spelled out, e.g. `a<CR><LF>`.
 *
 * Backslashes stay as they are, so a real newline can't be confused with an escape in a string.
 */
inline std::string show(std::string_view text) {
    std::string result = "`";
    for (const char c : text) {
        const auto byte = static_cast<unsigned char>(c);
        if (c == '\n') {
            result += "<LF>";
        } else if (c == '\r') {
            result += "<CR>";
        } else if (c == '\t') {
            result += "<TAB>";
        } else if (byte < 0x20 || byte >= 0x7F) {
            char hex[8];
            std::snprintf(hex, sizeof(hex), "<%02X>", byte);
            result += hex;
        } else {
            result += c;
        }
    }
    return result + "`";
}
