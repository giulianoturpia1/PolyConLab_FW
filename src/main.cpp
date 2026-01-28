#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "StateMachine.hpp"

QueueHandle_t eventQueue = nullptr;

void state_machine_task(void* pvParameters) {
    auto& sm = modules::StateMachine::instance();
    sm.begin();

    Serial.println("StateMachine task started");

    definitions::SystemEvent event;

    for (;;) {
        // Check for events (non-blocking, timeout of 10ms)
        if (xQueueReceive(eventQueue, &event, pdMS_TO_TICKS(10)) == pdPASS) {
            Serial.print("Event received: ");
            Serial.println(static_cast<int>(event));
            sm.process_event(event);

            // Log state transition
            Serial.print("Current state: ");
            Serial.println(sm.get_state_name(sm.get_current_state()).c_str());
        }

        sm.run();  // Always run state handlers
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void test_task(void* pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(2000));  // Wait 2 seconds before starting tests

    Serial.println("\n=== Starting StateMachine Tests ===\n");

    // Test 1: STARTUP -> CHECKUP transition
    Serial.println("Test 1: Sending INIT_OK event (STARTUP -> CHECKUP)");
    definitions::SystemEvent event = definitions::SystemEvent::INIT_OK;
    xQueueSend(eventQueue, &event, pdMS_TO_TICKS(100));
    vTaskDelay(pdMS_TO_TICKS(500));

    // Test 2: CHECKUP -> CALIBRATION transition
    Serial.println("Test 2: Sending START_CALIBRATION event (CHECKUP -> CALIBRATION)");
    event = definitions::SystemEvent::START_CALIBRATION;
    xQueueSend(eventQueue, &event, pdMS_TO_TICKS(100));
    vTaskDelay(pdMS_TO_TICKS(500));

    // Test 3: CALIBRATION -> CHECKUP transition
    Serial.println("Test 3: Sending FINISH_CALIBRATION event (CALIBRATION -> CHECKUP)");
    event = definitions::SystemEvent::FINISH_CALIBRATION;
    xQueueSend(eventQueue, &event, pdMS_TO_TICKS(100));
    vTaskDelay(pdMS_TO_TICKS(500));

    // Test 4: CHECKUP -> MANUAL (needs both SCADA and SENSORS ok)
    Serial.println("Test 4: Sending SCADA_LINK_OK and SENSORS_OK (CHECKUP -> MANUAL)");
    event = definitions::SystemEvent::SCADA_LINK_OK;
    xQueueSend(eventQueue, &event, pdMS_TO_TICKS(100));
    vTaskDelay(pdMS_TO_TICKS(100));
    event = definitions::SystemEvent::SENSORS_OK;
    xQueueSend(eventQueue, &event, pdMS_TO_TICKS(100));
    vTaskDelay(pdMS_TO_TICKS(500));

    // Test 5: MANUAL -> SOFT_ERROR transition
    Serial.println("Test 5: Sending FAULT_RECOVERABLE event (MANUAL -> SOFT_ERROR)");
    event = definitions::SystemEvent::FAULT_RECOVERABLE;
    xQueueSend(eventQueue, &event, pdMS_TO_TICKS(100));
    vTaskDelay(pdMS_TO_TICKS(500));

    // Test 6: SOFT_ERROR -> CRITICAL_ERROR transition
    Serial.println("Test 6: Sending FAULT_CRITICAL event (SOFT_ERROR -> CRITICAL_ERROR)");
    event = definitions::SystemEvent::FAULT_CRITICAL;
    xQueueSend(eventQueue, &event, pdMS_TO_TICKS(100));
    vTaskDelay(pdMS_TO_TICKS(500));

    // Test 7: CRITICAL_ERROR -> STARTUP transition
    Serial.println("Test 7: Sending CMD_RESET_FAULT event (CRITICAL_ERROR -> STARTUP)");
    event = definitions::SystemEvent::CMD_RESET_FAULT;
    xQueueSend(eventQueue, &event, pdMS_TO_TICKS(100));
    vTaskDelay(pdMS_TO_TICKS(500));

    Serial.println("\n=== Tests Complete ===\n");

    vTaskDelete(nullptr);
}

void setup() {
    Serial.begin(9600);
    delay(1000);

    Serial.println("\n=== PolyConLab Firmware ===\n");

    // Create event queue
    eventQueue = xQueueCreate(10, sizeof(definitions::SystemEvent));
    if (eventQueue == nullptr) {
        Serial.println("Failed to create event queue!");
        return;
    }

    // Create StateMachine task with high priority
    xTaskCreate(state_machine_task, "StateMachine", 4096, nullptr, tskIDLE_PRIORITY + 2, nullptr);

    // Create test task with lower priority
    xTaskCreate(test_task, "TestTask", 2048, nullptr, tskIDLE_PRIORITY + 1, nullptr);

    Serial.println("Setup complete, FreeRTOS tasks created\n");
}

void loop() {
    // FreeRTOS is running, loop is handled by the OS
    delay(10);
}
