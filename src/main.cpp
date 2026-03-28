#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "App.hpp"

void setup() {
    modules::App::instance().begin();
}

void loop() {}
