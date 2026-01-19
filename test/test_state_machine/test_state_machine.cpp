#include <Arduino.h>
#include <unity.h>

#include "StateMachine.hpp"

using definitions::SystemEvent;
using definitions::SystemState;

static modules::StateMachine& sm = modules::StateMachine::instance();

void setUp() {}
void tearDown() {}

void test_begin_sets_startup_state() {
    sm.begin();
    TEST_ASSERT_TRUE(sm.is_in_state(SystemState::STARTUP));
}

void test_startup_to_checkup_on_init_ok() {
    sm.begin();
    sm.process_event(SystemEvent::INIT_OK);
    TEST_ASSERT_TRUE(sm.is_in_state(SystemState::CHECKUP));
}

void test_checkup_to_calibration_on_start_calibration() {
    sm.begin();
    sm.process_event(SystemEvent::INIT_OK);
    sm.process_event(SystemEvent::START_CALIBRATION);
    TEST_ASSERT_TRUE(sm.is_in_state(SystemState::CALIBRATION));
}

void test_calibration_to_critical_on_fault_critical() {
    sm.begin();
    sm.process_event(SystemEvent::INIT_OK);
    sm.process_event(SystemEvent::START_CALIBRATION);
    sm.process_event(SystemEvent::FAULT_CRITICAL);
    TEST_ASSERT_TRUE(sm.is_in_state(SystemState::CRITICAL_ERROR));
}

void test_critical_to_startup_on_reset_fault() {
    sm.begin();
    sm.process_event(SystemEvent::INIT_OK);
    sm.process_event(SystemEvent::START_CALIBRATION);
    sm.process_event(SystemEvent::FAULT_CRITICAL);
    sm.process_event(SystemEvent::CMD_RESET_FAULT);
    TEST_ASSERT_TRUE(sm.is_in_state(SystemState::STARTUP));
}

void test_state_name_mapping() {
    TEST_ASSERT_EQUAL_STRING("STARTUP", modules::StateMachine::get_state_name(SystemState::STARTUP).c_str());
    TEST_ASSERT_EQUAL_STRING("CHECKUP", modules::StateMachine::get_state_name(SystemState::CHECKUP).c_str());
    TEST_ASSERT_EQUAL_STRING("CALIBRATION", modules::StateMachine::get_state_name(SystemState::CALIBRATION).c_str());
    TEST_ASSERT_EQUAL_STRING("MANUAL", modules::StateMachine::get_state_name(SystemState::MANUAL).c_str());
    TEST_ASSERT_EQUAL_STRING("SOFT_ERROR", modules::StateMachine::get_state_name(SystemState::SOFT_ERROR).c_str());
    TEST_ASSERT_EQUAL_STRING("CRITICAL_ERROR",
                             modules::StateMachine::get_state_name(SystemState::CRITICAL_ERROR).c_str());
}

void setup() {
    Serial.begin(9600);
    delay(500);
    UNITY_BEGIN();

    RUN_TEST(test_begin_sets_startup_state);
    RUN_TEST(test_startup_to_checkup_on_init_ok);
    RUN_TEST(test_checkup_to_calibration_on_start_calibration);
    RUN_TEST(test_calibration_to_critical_on_fault_critical);
    RUN_TEST(test_critical_to_startup_on_reset_fault);
    RUN_TEST(test_state_name_mapping);

    UNITY_END();
}

void loop() {
    delay(100);
}
