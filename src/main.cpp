#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "App.hpp"
#include "HALGeneral.hpp"

void setup() {
    modules::App::instance().begin();

    vTaskStartScheduler();

    hal_general.log("Error: El scheduler no se pudo iniciar.");
}

void loop() {}
