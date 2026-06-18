#include "HALGeneral.hpp"

#include "HALGeneralArduino.hpp"

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
    static HALGeneralArduino arduino_hal;
    hal_ = &arduino_hal;
}

}  // namespace modules
