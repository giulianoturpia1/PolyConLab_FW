///
/// @file Communications.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Communications module that isolates command handling from the application layer.
/// @version 0.1
/// @date 26-02-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include <functional>
#include <string>

namespace modules {

///
/// @brief Communications manager that routes commands from multiple transports.
/// Implemented as a singleton.
///
class Communications {
   public:
	///
	/// @brief Get the singleton instance of Communications
	///
	static Communications& instance();

	///
	/// @brief Set the callback for sending Ethernet responses
	///
	void set_ethernet_response_callback(const std::function<void(const std::string&)>& callback);

	///
	/// @brief Set the callback for sending serial responses
	///
	void set_serial_response_callback(const std::function<void(const std::string&)>& callback);

	///
	/// @brief Handle a message coming from Ethernet.
	///
    void handle_ethernet_message(const std::string& msg);

    ///
    /// @brief Handle a message coming from Serial.
    ///
    void handle_serial_message(const std::string& msg);

   private:
	Communications();
	// Rule of Five
	~Communications() = default;
	Communications(const Communications&) = delete;
	Communications& operator=(const Communications&) = delete;
	Communications(Communications&&) = delete;
	Communications& operator=(Communications&&) = delete;

    void handle_message(const std::string& msg, const std::function<void(const std::string&)>& respond);
    bool handle_device_command(const std::string& msg, const std::function<void(const std::string&)>& respond);
    bool handle_event_command(const std::string& msg, const std::function<void(const std::string&)>& respond);
    bool dispatch_event(const std::string& event_name);

    std::function<void(const std::string&)> ethernet_response_callback_;
    std::function<void(const std::string&)> serial_response_callback_;
};

}  // namespace modules
