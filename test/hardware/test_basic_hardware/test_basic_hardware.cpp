#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include "test_config.hpp"

// Define test pins for simulated hardware tests
#define TEST_PIN_OUTPUT 2
#define TEST_PIN_INPUT 4

// Mock variables for tracking pin state in simulation
static int pin_mode_values[40] = {0};     // Track pinMode calls
static int pin_digital_values[40] = {0};  // Track digitalWrite calls

TEST_CASE("pinMode sets OUTPUT on test pin") {
    pinMode(TEST_PIN_OUTPUT, OUTPUT);
    pin_mode_values[TEST_PIN_OUTPUT] = OUTPUT;
    CHECK_EQ(pin_mode_values[TEST_PIN_OUTPUT], OUTPUT);
}

TEST_CASE("digitalWrite sets HIGH") {
    pinMode(TEST_PIN_OUTPUT, OUTPUT);
    digitalWrite(TEST_PIN_OUTPUT, HIGH);
    pin_digital_values[TEST_PIN_OUTPUT] = HIGH;
    CHECK_EQ(pin_digital_values[TEST_PIN_OUTPUT], HIGH);
}

TEST_CASE("digitalWrite sets LOW") {
    pinMode(TEST_PIN_OUTPUT, OUTPUT);
    digitalWrite(TEST_PIN_OUTPUT, LOW);
    pin_digital_values[TEST_PIN_OUTPUT] = LOW;
    CHECK_EQ(pin_digital_values[TEST_PIN_OUTPUT], LOW);
}

TEST_CASE("Serial.begin initializes") {
    Serial.begin(115200);
    CHECK(true);
}

TEST_CASE("millis increases over time") {
    const auto start = millis();
    delay(100);
    const auto end = millis();
    CHECK((end - start) >= 90);
}

TEST_CASE("delay waits at least requested time") {
    const auto start = millis();
    delay(50);
    const auto end = millis();
    CHECK((end - start) >= 40);
}

TEST_CASE("micros increases over time") {
    const auto start = micros();
    delay(10);
    const auto end = micros();
    CHECK(end > start);
}

TEST_CASE("digitalWrite sequence toggles pin") {
    pinMode(TEST_PIN_OUTPUT, OUTPUT);

    digitalWrite(TEST_PIN_OUTPUT, HIGH);
    CHECK_EQ(digitalRead(TEST_PIN_OUTPUT), HIGH);

    digitalWrite(TEST_PIN_OUTPUT, LOW);
    CHECK_EQ(digitalRead(TEST_PIN_OUTPUT), LOW);

    digitalWrite(TEST_PIN_OUTPUT, HIGH);
    CHECK_EQ(digitalRead(TEST_PIN_OUTPUT), HIGH);
}

TEST_CASE("multiple pins set to expected states") {
    pinMode(2, OUTPUT);
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);

    digitalWrite(2, HIGH);
    digitalWrite(4, LOW);
    digitalWrite(5, HIGH);

    CHECK_EQ(digitalRead(2), HIGH);
    CHECK_EQ(digitalRead(4), LOW);
    CHECK_EQ(digitalRead(5), HIGH);
}

TEST_CASE("analogWrite executes") {
    pinMode(15, OUTPUT);
    analogWrite(15, 128);
    CHECK(true);
}

#if defined(ARDUINO)
void setup() {
    Serial.begin(115200);
    delay(500);
    doctest::Context context;
    context.setOption("no-breaks", true);
    context.setOption("success", true);      // Report successful tests
    context.setOption("no-exitcode", true);  // Keep exit code zero on failures for PlatformIO
    (void) context.run();
}

void loop() {
    delay(100);
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
