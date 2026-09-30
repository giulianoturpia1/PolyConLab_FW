///
/// @file Logger.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Runtime-toggleable leveled logger for the PolyConLab firmware
/// @version 0.1
/// @date 22-05-2026
///
/// @copyright Copyright (c) 2026
///
/// @details Provides a singleton logger that routes messages to the serial port
///          through hal_general().serial_println(). Each message is tagged with
///          a severity prefix: [DBG], [INF], [WRN], or [ERR].
///
///          Logging can be suppressed at runtime via set_enabled(false) or the
///          SCPI command SYST:LOG OFF, which is useful when a SCPI client shares
///          the serial line and must receive clean, untagged responses.
///
///          Use the logger() helper to access the singleton instance.
///

#pragma once

#include <cstring>
#include <string>

namespace modules {

///
/// @brief Leveled, runtime-toggleable firmware logger.
///
/// Singleton — access via logger(). Starts enabled or not per config::kLogEnabledAtBoot.
///
/// Output format: "[LVL] message\n" written to hal_general().serial_println().
/// All output is silenced when disabled; SCPI responses are unaffected.
///
class Logger {
   public:
    ///
    /// @brief Get the singleton Logger instance.
    ///
    /// @return Logger& Reference to the single Logger instance.
    ///
    static Logger& instance();

    ///
    /// @brief Enable or disable all log output at runtime.
    ///
    /// When disabled, calls to debug(), info(), warning(), and error() are
    /// no-ops. Can also be toggled via the SCPI commands SYST:LOG ON / OFF.
    ///
    /// @param enabled true to enable logging, false to suppress all output.
    ///
    void set_enabled(bool enabled);

    ///
    /// @brief Check whether logging is currently enabled.
    ///
    /// @return true if log output is active, false if suppressed.
    ///
    bool is_enabled() const;

    ///
    /// @brief Log a debug-level message (prefix [DBG]).
    ///
    /// Use for high-frequency trace output such as received bytes or internal
    /// state transitions that are only relevant during active development.
    ///
    /// @param msg Message to log.
    ///
    void debug(const std::string& msg);

    ///
    /// @brief Log an informational message (prefix [INF]).
    ///
    /// Use for normal operational events such as state entries, device
    /// initialisation, and network link changes.
    ///
    /// @param msg Message to log.
    ///
    void info(const std::string& msg);

    ///
    /// @brief Log a warning message (prefix [WRN]).
    ///
    /// Use for unexpected but recoverable conditions such as duplicate
    /// registrations, commands rejected by the current state, or
    /// re-entry guards firing.
    ///
    /// @param msg Message to log.
    ///
    void warning(const std::string& msg);

    ///
    /// @brief Log an error message (prefix [ERR]).
    ///
    /// Use for failures that impair normal operation: queue/task creation
    /// failures, invalid pins or addresses, and unrecoverable I2C errors.
    ///
    /// @param msg Message to log.
    ///
    void error(const std::string& msg);

   private:
    Logger();
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    bool enabled_;  ///< When false, write() is a no-op.

    ///
    /// @brief Internal sink: prepend prefix and forward to serial if enabled.
    ///
    /// @param prefix Level tag string (e.g. "[INF] ").
    /// @param msg    Message body.
    ///
    void write(const char* prefix, const std::string& msg);
};

///
/// @brief Convenience accessor for the Logger singleton.
///
/// @return Logger& Reference to the single Logger instance.
///
inline Logger& logger() {
    return Logger::instance();
}

///
/// @brief Lightweight per-module logger that prepends a module tag to every message.
///
/// Create one static instance per translation unit and use it in place of the
/// global logger() so that every log line identifies its origin module.
///
/// Output format: "[LVL] [Tag] message"  (e.g. "[ERR] [Heater] Pin invalido")
///
/// Usage in a .cpp file:
/// @code
///   static const TaggedLogger kLog("Heater");
///
///   kLog.error("Pin invalido para el calentador.");
/// @endcode
///
class TaggedLogger {
   public:
    ///
    /// @brief Construct a TaggedLogger bound to the given module name.
    ///
    /// @param tag Null-terminated module name. Must be a string literal or otherwise
    ///            outlive this object (lifetime matches the enclosing translation unit).
    ///
    explicit TaggedLogger(const char* tag) : tag_(tag) {}

    ///
    /// @brief Log a debug-level message tagged with this module's name (prefix [DBG]).
    ///
    /// @param msg Message body.
    ///
    void debug(const std::string& msg) const {
        if (logger().is_enabled())
            logger().debug(format(msg));
    }

    ///
    /// @brief Log an informational message tagged with this module's name (prefix [INF]).
    ///
    /// @param msg Message body.
    ///
    void info(const std::string& msg) const {
        if (logger().is_enabled())
            logger().info(format(msg));
    }

    ///
    /// @brief Log a warning tagged with this module's name (prefix [WRN]).
    ///
    /// @param msg Message body.
    ///
    void warning(const std::string& msg) const {
        if (logger().is_enabled())
            logger().warning(format(msg));
    }

    ///
    /// @brief Log an error tagged with this module's name (prefix [ERR]).
    ///
    /// @param msg Message body.
    ///
    void error(const std::string& msg) const {
        if (logger().is_enabled())
            logger().error(format(msg));
    }

   private:
    const char* tag_;  ///< Module name tag (must point to a string literal).

    /// @brief Format a message by prepending the module tag. Output format: "[Tag] message".
    /// @param msg Message body.
    /// @return Formatted message with module tag.
    std::string format(const std::string& msg) const {
        std::string result;
        result.reserve(1 + strlen(tag_) + 2 + msg.size());
        result += '[';
        result += tag_;
        result += "] ";
        result += msg;
        return result;
    }
};

}  // namespace modules
