#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "App.hpp"
#include "HALGeneral.hpp"
#include "Logger.hpp"

static const modules::TaggedLogger kLog("main");

void setup() {
    modules::App::instance().begin();

    vTaskStartScheduler();

    kLog.error("Error: El scheduler no se pudo iniciar.");
}

void loop() {}
