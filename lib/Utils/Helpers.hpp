///
/// @file Helpers.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Utility functions for common string operations
/// @version 0.1
/// @date 26-02-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>

#include "Definitions.hpp"

namespace utils {

///
/// @brief Trim whitespace from both ends of a string
///
/// @param str Input string
/// @return std::string Trimmed string
///
inline std::string trim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char ch) { return std::isspace(ch); });
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();
    return (start < end) ? std::string(start, end) : std::string();
}

///
/// @brief Check if a string starts with a given prefix
///
/// @param value String to check
/// @param prefix Prefix to look for
/// @return bool True if value starts with prefix
///
inline bool starts_with(const std::string& value, const char* prefix) {
    size_t prefix_len = std::strlen(prefix);
    return value.size() >= prefix_len && value.compare(0, prefix_len, prefix) == 0;
}

///
/// @brief Format a float with specified precision
///
/// @param value Float value to format
/// @param precision Number of decimal places
/// @return std::string Formatted string
///
inline std::string format_float(float value, int precision) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.*f", precision, value);
    return std::string(buffer);
}

///
/// @brief Find a command in the command map by name
///
/// @param cmd_name
/// @return const definitions::CommandMap*
///
inline const definitions::CommandMap* find_command(const std::string& cmd_name) {
    const auto it = std::find_if(definitions::kCommandMap.begin(), definitions::kCommandMap.end(),
                                 [&cmd_name](const definitions::CommandMap& entry) { return cmd_name == entry.name; });
    return it != definitions::kCommandMap.end() ? &(*it) : nullptr;
}

///
/// @brief Resolve a response text from the response map
///
/// @param type Response identifier
/// @return const char* Canonical response text
///
inline const char* response(const definitions::ResponseType type) {
    const auto it =
        std::find_if(definitions::kResponseMap.begin(), definitions::kResponseMap.end(),
                     [type](const definitions::ResponseMap& response_entry) { return response_entry.type == type; });
    if (it != definitions::kResponseMap.end()) {
        return it->name;
    }

    return "";
}

}  // namespace utils
