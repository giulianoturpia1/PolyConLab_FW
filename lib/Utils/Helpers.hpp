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
#include <cmath>
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
/// @brief Look up a SCPI command string in the command table.
///
/// @param cmd_name SCPI command string to search for (e.g. "OUTP:HEAT ON").
/// @return const definitions::CommandEntry* Pointer to the matching entry, or nullptr if not found.
///
inline const definitions::CommandEntry* find_command(const std::string& cmd_name) {
    const auto it =
        std::find_if(definitions::kCommandMap.begin(), definitions::kCommandMap.end(),
                     [&cmd_name](const definitions::CommandEntry& entry) { return entry.name != nullptr && cmd_name == entry.name; });
    return it != definitions::kCommandMap.end() ? &(*it) : nullptr;
}

///
/// @brief Look up a SCPI event string in the event table.
///
/// @param event_name SCPI event string to search for (e.g. "EVT:PAUSE").
/// @return const definitions::EventEntry* Pointer to the matching entry, or nullptr if not found.
///
inline const definitions::EventEntry* find_event(const std::string& event_name) {
    const auto it = std::find_if(definitions::kEventMap.begin(), definitions::kEventMap.end(),
                                 [&event_name](const definitions::EventEntry& entry) { return event_name == entry.name; });
    return it != definitions::kEventMap.end() ? &(*it) : nullptr;
}

///
/// @brief Resolve a ResponseType to its canonical SCPI wire string.
///
/// @param type Response type identifier.
/// @return const char* Wire string (e.g. "ON", "-100,'Command error'"), or "ERROR:UNKNOWN_RESPONSE" if not found.
///
inline const char* to_scpi(const definitions::ResponseType type) {
    const auto it = std::find_if(definitions::kResponseMap.begin(), definitions::kResponseMap.end(),
                                 [type](const definitions::ResponseEntry& entry) { return entry.name != nullptr && entry.type == type; });
    return it != definitions::kResponseMap.end() ? it->name : "ERROR:UNKNOWN_RESPONSE";
}

///
/// @brief Resolve a CommandType to its canonical SCPI response wire string.
///
/// Looks up the command in kCommandResponseMap to find its ResponseType, then resolves
/// that to the wire string via to_scpi(ResponseType). Returns the unknown-command
/// error string if the command has no registered response.
///
/// @param command Command type to look up.
/// @return const char* Corresponding SCPI response wire string.
///
inline const char* to_scpi(const definitions::CommandType command) {
    const auto it = std::find_if(definitions::kCommandResponseMap.begin(), definitions::kCommandResponseMap.end(),
                                 [command](const definitions::CommandResponseEntry& entry) { return entry.command == command; });
    return it != definitions::kCommandResponseMap.end() ? to_scpi(it->response) : to_scpi(definitions::ResponseType::kErrorUnknownCommand);
}

///
/// @brief Look up the display name of a SystemState.
///
/// @param state System state to look up.
/// @return const char* Human-readable state name (e.g. "MANUAL"), or "UNKNOWN" if not found.
///
inline const char* state_name(const definitions::SystemState state) {
    const auto it = std::find_if(definitions::kStateNameMap.begin(), definitions::kStateNameMap.end(),
                                 [state](const definitions::StateNameEntry& entry) { return entry.state == state; });
    return it != definitions::kStateNameMap.end() ? it->name : "UNKNOWN";
}

///
/// @brief Look up the SCPI command string for a given CommandType.
///
/// Reverses the kCommandMap lookup — useful for logging a command name
/// without coupling to SCPI wire format concerns.
///
/// @param command Command type to look up.
/// @return const char* SCPI command string (e.g. "OUTP:HEAT ON"), or "UNKNOWN" if not found.
///
inline const char* command_name(const definitions::CommandType command) {
    const auto it = std::find_if(definitions::kCommandMap.begin(), definitions::kCommandMap.end(),
                                 [command](const definitions::CommandEntry& entry) { return entry.command == command; });
    return it != definitions::kCommandMap.end() ? it->name : "UNKNOWN";
}

///
/// @brief Whether a message sender holds operator privilege (pure authorization policy).
///
/// Serial is the local physical console and is always privileged. Over WiFi, only the client
/// holding the operator role may actuate; operator_id == 0 means the role is unclaimed.
///
/// @param transport   Transport the message arrived on.
/// @param sender      Identity of the sending client (0 for serial).
/// @param operator_id Client currently holding the operator role (0 = unclaimed).
/// @return bool True if the sender holds operator privilege.
///
inline bool sender_is_operator(const definitions::Transport transport, const uint32_t sender, const uint32_t operator_id) {
    if (transport == definitions::Transport::kSerial) {
        return true;
    }
    return operator_id != 0 && sender == operator_id;
}

///
/// @brief Format an integer value as a "0xNN" hex string.
///
/// @param value Value to format.
/// @return std::string Hex string (e.g. "0x60").
///
inline std::string format_hex(uint32_t value) {
    char buf[11];
    snprintf(buf, sizeof(buf), "0x%02X", value);
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
/// @brief Type-pun a float into its raw uint32_t bit pattern, for packing into Command::extra.
///
/// @param value Float to encode.
/// @return uint32_t IEEE-754 bit pattern of @p value.
///
inline uint32_t float_to_bits(float value) {
    static_assert(sizeof(float) == sizeof(uint32_t), "float/uint32_t size mismatch");
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

///
/// @brief Inverse of @c float_to_bits: recover a float from its raw uint32_t bit pattern.
///
/// @param bits IEEE-754 bit pattern (as carried in Command::extra).
/// @return float Decoded value.
///
inline float bits_to_float(uint32_t bits) {
    static_assert(sizeof(float) == sizeof(uint32_t), "float/uint32_t size mismatch");
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

///
/// @brief Unpack an IEEE-754 float stored in a uint32_t and format it to 2 decimal places.
///
/// Sensor modules pack float readings into Command::extra via memcpy. This function
/// reverses that packing and formats the result for the SCPI wire format.
///
/// @param bits Raw uint32_t holding the IEEE-754 bit pattern of a float.
/// @return std::string Formatted value (e.g. "25.50") or SCPI error string.
///
inline std::string format_sensor_value(uint32_t bits) {
    float val = bits_to_float(bits);
    if (std::isnan(val)) {
        return to_scpi(definitions::ResponseType::kErrorSensorFault);
    }
    return format_float(val, 2);
}

}  // namespace utils
