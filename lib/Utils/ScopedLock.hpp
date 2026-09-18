///
/// @file ScopedLock.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief RAII guard for a FreeRTOS mutex. Acquires on construction, releases on scope exit.
/// @version 0.1
/// @date 14-08-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace modules {

///
/// @brief Scope-bound lock over a FreeRTOS mutex (created with @c xSemaphoreCreateMutex,
/// so it carries priority inheritance).
///
/// Take the mutex with a bounded timeout on construction and always release it on
/// destruction. A null handle or a failed take leaves the guard un-held: callers must
/// check held() before touching the protected resource. The lock never blocks longer
/// than the timeout, so a wedged holder cannot stall the caller indefinitely.
///
class ScopedLock {
   public:
    ///
    /// @brief Attempt to take @p mutex, waiting at most @p timeout_ticks.
    ///
    /// @param mutex Mutex handle, or nullptr to make this a no-op un-held guard.
    /// @param timeout_ticks Maximum ticks to wait for acquisition.
    ///
    ScopedLock(SemaphoreHandle_t mutex, TickType_t timeout_ticks)
        : mutex_(mutex), held_(mutex != nullptr && xSemaphoreTake(mutex, timeout_ticks) == pdTRUE) {}

    ~ScopedLock() {
        if (held_) {
            xSemaphoreGive(mutex_);
        }
    }

    ///
    /// @brief Whether the mutex is currently held by this guard.
    ///
    /// @return true if acquisition succeeded; false otherwise (do not touch the resource).
    ///
    bool held() const {
        return held_;
    }

    // Rule of Five: a scope guard is neither copyable nor movable.
    ScopedLock(const ScopedLock&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;
    ScopedLock(ScopedLock&&) = delete;
    ScopedLock& operator=(ScopedLock&&) = delete;

   private:
    SemaphoreHandle_t mutex_;  ///< Guarded mutex handle (may be nullptr).
    bool held_;                ///< True when this guard owns the mutex.
};

}  // namespace modules
