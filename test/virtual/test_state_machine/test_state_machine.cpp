#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include "StateMachine.hpp"
#include "test_config.hpp"

using definitions::SystemEvent;
using definitions::SystemState;

static modules::StateMachine& sm = modules::StateMachine::instance();

TEST_CASE("StateMachine starts in STARTUP after begin()") {
    sm.begin();
    CHECK(sm.is_in_state(SystemState::STARTUP));
}

TEST_CASE("INIT_OK transitions STARTUP -> CHECKUP") {
    sm.begin();
    sm.process_event(SystemEvent::INIT_OK);
    CHECK(sm.is_in_state(SystemState::CHECKUP));
}

TEST_CASE("START_CALIBRATION transitions CHECKUP -> CALIBRATION") {
    sm.begin();
    sm.process_event(SystemEvent::INIT_OK);
    sm.process_event(SystemEvent::START_CALIBRATION);
    CHECK(sm.is_in_state(SystemState::CALIBRATION));
}

TEST_CASE("FAULT_CRITICAL transitions to CRITICAL_ERROR") {
    sm.begin();
    sm.process_event(SystemEvent::INIT_OK);
    sm.process_event(SystemEvent::START_CALIBRATION);
    sm.process_event(SystemEvent::FAULT_CRITICAL);
    CHECK(sm.is_in_state(SystemState::CRITICAL_ERROR));
}

TEST_CASE("CMD_RESET_FAULT transitions CRITICAL_ERROR -> STARTUP") {
    sm.begin();
    sm.process_event(SystemEvent::INIT_OK);
    sm.process_event(SystemEvent::START_CALIBRATION);
    sm.process_event(SystemEvent::FAULT_CRITICAL);
    sm.process_event(SystemEvent::CMD_RESET_FAULT);
    CHECK(sm.is_in_state(SystemState::STARTUP));
}

TEST_CASE("state name mapping returns expected labels") {
    CHECK_EQ(modules::StateMachine::get_state_name(SystemState::STARTUP), "STARTUP");
    CHECK_EQ(modules::StateMachine::get_state_name(SystemState::CHECKUP), "CHECKUP");
    CHECK_EQ(modules::StateMachine::get_state_name(SystemState::CALIBRATION), "CALIBRATION");
    CHECK_EQ(modules::StateMachine::get_state_name(SystemState::MANUAL), "MANUAL");
    CHECK_EQ(modules::StateMachine::get_state_name(SystemState::SOFT_ERROR), "SOFT_ERROR");
    CHECK_EQ(modules::StateMachine::get_state_name(SystemState::CRITICAL_ERROR), "CRITICAL_ERROR");
}

#if defined(ARDUINO)
void setup() {
    delay(500);
    doctest::Context context;
    context.setOption("no-breaks", true);
    context.setOption("success", true);      // Report successful tests
    context.setOption("no-exitcode", true);  // Keep exit code zero on failures for PlatformIO
    (void) context.run();
}

void loop() {
    // Nothing to do here
}
#else
int main(int argc, char** argv) {
    doctest::Context context;
    context.applyCommandLine(argc, argv);
    context.setOption("success", true);      // Report successful tests
    context.setOption("no-exitcode", true);  // Keep exit code zero on failures for PlatformIO
    return context.run();
}
#endif
