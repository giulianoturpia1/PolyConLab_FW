#include "FaultRegistry.hpp"

namespace modules {

FaultRegistry& FaultRegistry::instance() {
    static FaultRegistry inst;
    return inst;
}

void FaultRegistry::clear() {
    faults_ = 0;
}

void FaultRegistry::add(definitions::FaultId id) {
    faults_ |= (1u << static_cast<uint8_t>(id));
}

void FaultRegistry::remove(definitions::FaultId id) {
    faults_ &= ~(1u << static_cast<uint8_t>(id));
}

bool FaultRegistry::contains(definitions::FaultId id) const {
    return (faults_ & (1u << static_cast<uint8_t>(id))) != 0;
}

std::string FaultRegistry::get() const {
    if (faults_ == 0) {
        return "NONE";
    }

    std::string result;
    for (const definitions::FaultNameEntry& entry : definitions::kFaultNameMap) {
        if (!contains(entry.id)) {
            continue;
        }
        if (!result.empty()) {
            result += ",";
        }
        result += entry.name;
    }
    return result;
}

bool FaultRegistry::has_faults() const {
    return faults_ != 0;
}

}  // namespace modules
