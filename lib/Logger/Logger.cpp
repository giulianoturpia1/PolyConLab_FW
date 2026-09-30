#include "Logger.hpp"

#include "Config.hpp"
#include "HALGeneral.hpp"

namespace modules {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : enabled_(config::kLogEnabledAtBoot) {}

void Logger::set_enabled(bool enabled) {
    enabled_ = enabled;
}

bool Logger::is_enabled() const {
    return enabled_;
}

void Logger::debug(const std::string& msg) {
    write("[DBG] ", msg);
}

void Logger::info(const std::string& msg) {
    write("[INF] ", msg);
}

void Logger::warning(const std::string& msg) {
    write("[WRN] ", msg);
}

void Logger::error(const std::string& msg) {
    write("[ERR] ", msg);
}

void Logger::write(const char* prefix, const std::string& msg) {
    if (!enabled_) {
        return;
    }
    hal_general().serial_println(std::string(prefix) + msg);
}

}  // namespace modules
