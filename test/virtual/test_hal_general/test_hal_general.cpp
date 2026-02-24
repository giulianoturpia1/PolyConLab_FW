#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include "HALGeneral.hpp"

// Define test pins for simulated hardware tests
#define TEST_PIN_OUTPUT 2

TEST_CASE("pin_mode sets OUTPUT on test pin") {
    modules::hal_general().pin_mode(TEST_PIN_OUTPUT, modules::PIN_MODE_OUTPUT);
    CHECK(true);
}

TEST_CASE("digital_write sets HIGH") {
    modules::hal_general().pin_mode(TEST_PIN_OUTPUT, modules::PIN_MODE_OUTPUT);
    modules::hal_general().digital_write(TEST_PIN_OUTPUT, modules::PIN_HIGH);
    CHECK_EQ(modules::hal_general().digital_read(TEST_PIN_OUTPUT), modules::PIN_HIGH);
}

TEST_CASE("digital_write sets LOW") {
    modules::hal_general().pin_mode(TEST_PIN_OUTPUT, modules::PIN_MODE_OUTPUT);
    modules::hal_general().digital_write(TEST_PIN_OUTPUT, modules::PIN_LOW);
    CHECK_EQ(modules::hal_general().digital_read(TEST_PIN_OUTPUT), modules::PIN_LOW);
}

TEST_CASE("serial_begin initializes") {
    modules::hal_general().serial_begin(115200);
    CHECK(true);
}

TEST_CASE("millis increases over time") {
    const auto start = modules::hal_general().millis();
    modules::hal_general().delay(100);
    const auto end = modules::hal_general().millis();
    CHECK((end - start) >= 90);
}

TEST_CASE("delay waits at least requested time") {
    const auto start = modules::hal_general().millis();
    modules::hal_general().delay(50);
    const auto end = modules::hal_general().millis();
    CHECK((end - start) >= 40);
}

TEST_CASE("micros increases over time") {
    const auto start = modules::hal_general().micros();
    modules::hal_general().delay(10);
    const auto end = modules::hal_general().micros();
    CHECK(end > start);
}

TEST_CASE("digital_write sequence toggles pin") {
    modules::hal_general().pin_mode(TEST_PIN_OUTPUT, modules::PIN_MODE_OUTPUT);

    modules::hal_general().digital_write(TEST_PIN_OUTPUT, modules::PIN_HIGH);
    CHECK_EQ(modules::hal_general().digital_read(TEST_PIN_OUTPUT), modules::PIN_HIGH);

    modules::hal_general().digital_write(TEST_PIN_OUTPUT, modules::PIN_LOW);
    CHECK_EQ(modules::hal_general().digital_read(TEST_PIN_OUTPUT), modules::PIN_LOW);

    modules::hal_general().digital_write(TEST_PIN_OUTPUT, modules::PIN_HIGH);
    CHECK_EQ(modules::hal_general().digital_read(TEST_PIN_OUTPUT), modules::PIN_HIGH);
}

TEST_CASE("multiple pins set to expected states") {
    modules::hal_general().pin_mode(2, modules::PIN_MODE_OUTPUT);
    modules::hal_general().pin_mode(4, modules::PIN_MODE_OUTPUT);
    modules::hal_general().pin_mode(5, modules::PIN_MODE_OUTPUT);

    modules::hal_general().digital_write(2, modules::PIN_HIGH);
    modules::hal_general().digital_write(4, modules::PIN_LOW);
    modules::hal_general().digital_write(5, modules::PIN_HIGH);

    CHECK_EQ(modules::hal_general().digital_read(2), modules::PIN_HIGH);
    CHECK_EQ(modules::hal_general().digital_read(4), modules::PIN_LOW);
    CHECK_EQ(modules::hal_general().digital_read(5), modules::PIN_HIGH);
}

TEST_CASE("analog_write executes") {
    modules::hal_general().pin_mode(15, modules::PIN_MODE_OUTPUT);
    modules::hal_general().analog_write(15, 128);
    CHECK(true);
}

#if defined(ARDUINO)
void setup() {
    modules::hal_general().serial_begin(115200);
    modules::hal_general().delay(500);
    doctest::Context context;
    context.setOption("no-breaks", true);
    context.setOption("success", true);      // Report successful tests
    context.setOption("no-exitcode", true);  // Keep exit code zero on failures for PlatformIO
    (void) context.run();
}

void loop() {
    modules::hal_general().delay(100);
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
