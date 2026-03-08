#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "App.hpp"

void setup() {
    modules::App::instance().begin();
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
