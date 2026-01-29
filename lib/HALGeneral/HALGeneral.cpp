#include "HALGeneral.hpp"

// Platform-specific implementations
#if defined(ARDUINO)
#include "HALGeneralArduino.hpp"
#else
#include "HALGeneralVirtual.hpp"
#endif

namespace modules {

////////////////////////////////////// Public Methods //////////////////////////////////////

HALGeneralManager& HALGeneralManager::instance() {
    static HALGeneralManager instance;
    return instance;
}

HALGeneral& HALGeneralManager::get() {
    // HALGeneral is already initialized in constructor
    return *hal_;
}

void HALGeneralManager::set(HALGeneral* hal) {
    // Allow custom HALGeneral injection for testing
    hal_ = hal;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

HALGeneralManager::HALGeneralManager() : hal_(nullptr) {
    // Eager initialization - platform is known at compile time
    // Create the appropriate HALGeneral implementation immediately
#if defined(ARDUINO)
    // Arduino platform
    static HALGeneralArduino arduino_hal;
    hal_ = &arduino_hal;
#else
    // Virtual/Test platform
    static HALGeneralVirtual virtual_hal;
    hal_ = &virtual_hal;
#endif
}

}  // namespace modules
