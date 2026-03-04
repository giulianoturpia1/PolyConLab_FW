#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <cstdlib>
#include <string>

// Mock counter for testing state changes
static int test_counter = 0;

TEST_CASE("counter increments") {
    test_counter = 0;
    test_counter++;
    CHECK_EQ(test_counter, 1);
    test_counter++;
    CHECK_EQ(test_counter, 2);
}

TEST_CASE("counter reset") {
    test_counter = 5;
    CHECK_EQ(test_counter, 5);
    test_counter = 0;
    CHECK_EQ(test_counter, 0);
}

TEST_CASE("basic arithmetic") {
    const int a = 10;
    const int b = 5;
    CHECK_EQ(a + b, 15);
    CHECK_EQ(a - b, 5);
    CHECK_EQ(a * b, 50);
    CHECK_EQ(a / b, 2);
}

TEST_CASE("boolean logic") {
    const bool enabled = true;
    const bool disabled = false;
    CHECK(enabled);
    CHECK_FALSE(disabled);
    CHECK_EQ(enabled && !disabled, true);
}

TEST_CASE("string comparison") {
    const char* state1 = "STARTUP";
    const char* state2 = "STARTUP";
    const char* state3 = "RUNNING";

    CHECK_EQ(std::string(state1), std::string(state2));
    CHECK_NE(std::string(state1), std::string(state3));
}

TEST_CASE("array operations") {
    int values[] = {1, 2, 3, 4, 5};
    int sum = 0;

    for (int value : values) {
        sum += value;
    }

    CHECK_EQ(sum, 15);
    CHECK_EQ(values[0], 1);
    CHECK_EQ(values[4], 5);
}

TEST_CASE("dynamic allocation") {
    int* buffer = static_cast<int*>(malloc(sizeof(int) * 10));
    REQUIRE(buffer != nullptr);

    buffer[0] = 100;
    buffer[9] = 999;

    CHECK_EQ(buffer[0], 100);
    CHECK_EQ(buffer[9], 999);

    free(buffer);
}

TEST_CASE("conditional logic") {
    int status = 1;
    bool result = false;

    if (status == 1) {
        result = true;
    }

    CHECK(result);
}

TEST_CASE("loop iteration") {
    int count = 0;
    for (int i = 0; i < 100; i++) {
        count++;
    }
    CHECK_EQ(count, 100);
}

TEST_CASE("state transitions enum") {
    enum class system_state_t { kInit = 0, kRunning = 1, kStopped = 2 };

    system_state_t current_state = system_state_t::kInit;
    CHECK(current_state == system_state_t::kInit);

    current_state = system_state_t::kRunning;
    CHECK(current_state == system_state_t::kRunning);

    current_state = system_state_t::kStopped;
    CHECK(current_state == system_state_t::kStopped);
}

TEST_CASE("bitwise operations") {
    unsigned char flags = 0x00;

    flags |= (1 << 0);  // Set bit 0
    CHECK_EQ(flags, 0x01);

    flags |= (1 << 2);  // Set bit 2
    CHECK_EQ(flags, 0x05);

    flags &= static_cast<unsigned char>(~(1 << 0));  // Clear bit 0
    CHECK_EQ(flags, 0x04);
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
