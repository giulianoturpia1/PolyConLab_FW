///
/// @file FaultRegistry.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Singleton registry of devices that failed hardware initialisation.
/// @version 0.2
/// @date 17-09-2026
///
/// @copyright Copyright (c) 2026
///
/// App::initialize_devices() writes to this registry when a device's begin() returns false.
/// Communications reads from it to answer SYST:FAULT? queries. Unlike the original append-only
/// design, the registry is now mutable at runtime: the StateMachine task clears a device's fault
/// when a SYST:REINIT:* command for that device succeeds, so a single sensor that failed at boot
/// no longer keeps the whole board degraded (e.g. locked out of MANUAL) for the rest of the power
/// cycle.
///
/// Concurrency: storage is a single aligned uint32_t bitmask (one bit per definitions::FaultId).
/// Boot-time registration (add()/clear()) runs on the App/startup path before the StateMachine
/// task exists, so there is no concurrent writer during that phase. After boot, the only writer is
/// the StateMachine task (see StateMachine::process_response(), which calls remove() on a
/// successful re-init). Readers are Communications (App task, answering SYST:FAULT?) and
/// ManualState::tick() (StateMachine task). With a single writer and a naturally-aligned 32-bit
/// word on the ESP32 (Xtensa), a reader on another task can only ever observe the bitmask before or
/// after a given add()/remove() call, never a torn intermediate value, so no mutex is needed here.
/// Do NOT add a FreeRTOS dependency to this file. If a second concurrent writer is ever introduced
/// (e.g. two tasks both calling add()/remove()), this invariant breaks and faults_ must be guarded
/// (e.g. a portMUX_TYPE spinlock or a mutex) or made a std::atomic<uint32_t>.
///

#pragma once

#include <cstdint>
#include <string>

#include "Definitions.hpp"

namespace modules {

class FaultRegistry {
   public:
    static FaultRegistry& instance();

    /// @brief Remove all recorded faults (called before each initialisation pass).
    void clear();

    /// @brief Record a failed device.
    void add(definitions::FaultId id);

    /// @brief Clear a single device's fault (called when its re-initialisation succeeds).
    void remove(definitions::FaultId id);

    /// @brief Return true if @p id is currently recorded as faulted.
    bool contains(definitions::FaultId id) const;

    /// @brief Return a comma-separated fault list in FaultId declaration order, or "NONE" if empty.
    std::string get() const;

    /// @brief Return true if at least one fault is currently recorded.
    bool has_faults() const;

   private:
    FaultRegistry() = default;

    // Rule of Five
    ~FaultRegistry() = default;
    FaultRegistry(const FaultRegistry&) = delete;
    FaultRegistry& operator=(const FaultRegistry&) = delete;
    FaultRegistry(FaultRegistry&&) = delete;
    FaultRegistry& operator=(FaultRegistry&&) = delete;

    uint32_t faults_ = 0;  ///< Bitmask; bit N corresponds to definitions::FaultId value N.
};

}  // namespace modules
