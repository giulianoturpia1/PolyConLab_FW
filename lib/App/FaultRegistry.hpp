///
/// @file FaultRegistry.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Singleton registry of devices that failed hardware initialisation.
/// @version 0.1
/// @date 08-06-2026
///
/// @copyright Copyright (c) 2026
///
/// App::initialize_devices() writes to this registry when a device's begin()
/// returns false.  Communications reads from it to answer SYST:FAULT? queries.
/// The registry is append-only after boot; it reflects the hardware state at
/// startup and does not change during normal operation.
///

#pragma once

#include <string>

namespace modules {

class FaultRegistry {
   public:
    static FaultRegistry& instance();

    /// @brief Remove all recorded faults (called before each initialisation pass).
    void clear();

    /// @brief Record a failed device by its canonical name (e.g. "TEMP_IN").
    void add(const std::string& device_name);

    /// @brief Return a comma-separated fault list, or "NONE" if no faults.
    std::string get() const;

    /// @brief Return true if at least one fault has been recorded.
    bool has_faults() const;

   private:
    FaultRegistry() = default;
    std::string faults_;
};

}  // namespace modules
