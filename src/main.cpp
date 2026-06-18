#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "App.hpp"
#include "Logger.hpp"

static const modules::TaggedLogger kLog("main");

void setup() {
    modules::App::instance().begin();
}

void loop() {}
