#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include "StateMachine.hpp"

using definitions::SystemEvent;
using definitions::SystemState;

static modules::StateMachine& sm = modules::StateMachine::instance();

TEST_CASE("StateMachine starts in STARTUP after initialize()") {
    sm.initialize();
    CHECK(sm.is_in_state(SystemState::kStartup));
}

TEST_CASE("INIT_OK transitions STARTUP -> CHECKUP") {
    sm.initialize();
    sm.process_event(SystemEvent::kInitOk);
    CHECK(sm.is_in_state(SystemState::kCheckup));
}

TEST_CASE("START_CALIBRATION transitions CHECKUP -> CALIBRATION") {
    sm.initialize();
    sm.process_event(SystemEvent::kInitOk);
    sm.process_event(SystemEvent::kStartCalibration);
    CHECK(sm.is_in_state(SystemState::kCalibration));
}

TEST_CASE("FAULT_CRITICAL transitions to CRITICAL_ERROR") {
    sm.initialize();
    sm.process_event(SystemEvent::kInitOk);
    sm.process_event(SystemEvent::kStartCalibration);
    sm.process_event(SystemEvent::kFaultCritical);
    CHECK(sm.is_in_state(SystemState::kCriticalError));
}

TEST_CASE("CMD_RESET_FAULT transitions CRITICAL_ERROR -> STARTUP") {
    sm.initialize();
    sm.process_event(SystemEvent::kInitOk);
    sm.process_event(SystemEvent::kStartCalibration);
    sm.process_event(SystemEvent::kFaultCritical);
    sm.process_event(SystemEvent::kCmdResetFault);
    CHECK(sm.is_in_state(SystemState::kStartup));
}

TEST_CASE("state name mapping returns expected labels") {
    CHECK_EQ(modules::StateMachine::get_state_name(SystemState::kStartup), "STARTUP");
    CHECK_EQ(modules::StateMachine::get_state_name(SystemState::kCheckup), "CHECKUP");
    CHECK_EQ(modules::StateMachine::get_state_name(SystemState::kCalibration), "CALIBRATION");
    CHECK_EQ(modules::StateMachine::get_state_name(SystemState::kManual), "MANUAL");
    CHECK_EQ(modules::StateMachine::get_state_name(SystemState::kSoftError), "SOFT_ERROR");
    CHECK_EQ(modules::StateMachine::get_state_name(SystemState::kCriticalError), "CRITICAL_ERROR");
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
