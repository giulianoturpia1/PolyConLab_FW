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
#include <cstdio>
#include <cstring>
#include <string>

#include "Definitions.hpp"

namespace utils {

///
/// @brief Trim whitespace from both ends of a string.
///
/// @param str Input string.
/// @return std::string Trimmed string.
///
inline std::string trim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char ch) { return std::isspace(ch); });
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();
    return (start < end) ? std::string(start, end) : std::string();
}

///
/// @brief Check if a string starts with a given prefix.
///
/// @param value String to check.
/// @param prefix Prefix to look for.
/// @return bool True if value starts with prefix.
///
inline bool starts_with(const std::string& value, const char* prefix) {
    if (prefix == nullptr) {
        return false;
    }
    const size_t prefix_len = std::strlen(prefix);
    return value.size() >= prefix_len && value.compare(0, prefix_len, prefix) == 0;
}

///
/// @brief Format an I2C address as a "0xNN" hex string.
///
inline std::string hex_address(uint32_t address) {
    char buf[11];
    snprintf(buf, sizeof(buf), "0x%02X", address);
    return buf;
}

///
/// @brief Format a float with specified precision.
///
/// @param value Float value to format.
/// @param precision Number of decimal places.
/// @return std::string Formatted string.
///
inline std::string format_float(float value, int precision) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.*f", precision, value);
    return std::string(buffer);
}

///
/// @brief Find a command in the command map by name.
///
/// @param cmd_name Command name to search for.
/// @return const definitions::CommandMap* Pointer to the CommandMap entry if found, nullptr otherwise.
///
inline const definitions::CommandMap* find_command(const std::string& cmd_name) {
    const auto it =
        std::find_if(definitions::kCommandMap.begin(), definitions::kCommandMap.end(),
                     [&cmd_name](const definitions::CommandMap& entry) { return entry.name != nullptr && cmd_name == entry.name; });
    return it != definitions::kCommandMap.end() ? &(*it) : nullptr;
}

///
/// @brief Find an event in the event map by name.
///
/// @param event_name Event name to search for.
/// @return const definitions::EventMap* Pointer to the EventMap entry if found, nullptr otherwise.
///
inline const definitions::EventMap* find_event(const std::string& event_name) {
    const auto it = std::find_if(definitions::kEventMap.begin(), definitions::kEventMap.end(),
                                 [&event_name](const definitions::EventMap& entry) { return event_name == entry.name; });
    return it != definitions::kEventMap.end() ? &(*it) : nullptr;
}

///
/// @brief Resolve a response text from a response type enum.
///
/// @param response_type Response identifier
/// @return const char* Response text, or "ERROR:UNKNOWN_RESPONSE" if the type is not recognized.
///
inline const char* response(const definitions::ResponseType response_type) {
    const auto it = std::find_if(definitions::kResponseMap.begin(), definitions::kResponseMap.end(),
                                 [response_type](const definitions::ResponseMap& response_entry) {
                                     return response_entry.name != nullptr && response_entry.type == response_type;
                                 });
    if (it != definitions::kResponseMap.end()) {
        return it->name;
    }

    return "ERROR:UNKNOWN_RESPONSE";
}

///
/// @brief Resolve response text from command type enum
///
/// It returns the corresponding response for known commands, or a generic error response if the command is not recognized.
///
/// @param command Command identifier
/// @return const char* Response text corresponding to the command, or "ERROR:UNKNOWN_COMMAND" if the command is not recognized.
///
inline const char* response_for_command(const definitions::CommandType command) {
    const auto it = std::find_if(definitions::kCommandResponseMap.begin(), definitions::kCommandResponseMap.end(),
                                 [command](const definitions::CommandResponseMap& entry) { return entry.command == command; });
    // This step translates the command into a response type enummerate, which is then resolved to a response string using the response()
    // function.
    if (it != definitions::kCommandResponseMap.end()) {
        return response(it->response);
    }

    return response(definitions::ResponseType::kErrorUnknownCommand);
}

}  // namespace utils
