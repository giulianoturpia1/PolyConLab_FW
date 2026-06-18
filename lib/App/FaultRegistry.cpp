#include "FaultRegistry.hpp"

namespace modules {

FaultRegistry& FaultRegistry::instance() {
    static FaultRegistry inst;
    return inst;
}

void FaultRegistry::clear() {
    faults_.clear();
}

void FaultRegistry::add(const std::string& device_name) {
    if (!faults_.empty()) {
        faults_ += ",";
    }
    faults_ += device_name;
}

std::string FaultRegistry::get() const {
    return faults_.empty() ? "NONE" : faults_;
}

bool FaultRegistry::has_faults() const {
    return !faults_.empty();
}

}  // namespace modules
