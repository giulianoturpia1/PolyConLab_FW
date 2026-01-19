#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <unity.h>

static QueueHandle_t q = nullptr;
static volatile int produced = 0;
static volatile int consumed = 0;

static void producer_task(void* pvParameters) {
    for (int i = 0; i < 10; ++i) {
        if (xQueueSend(q, &i, pdMS_TO_TICKS(100)) == pdPASS) {
            ++produced;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    vTaskDelete(nullptr);
}

static void consumer_task(void* pvParameters) {
    int v = 0;
    for (int i = 0; i < 10; ++i) {
        if (xQueueReceive(q, &v, pdMS_TO_TICKS(200)) == pdPASS) {
            ++consumed;
        }
    }
    vTaskDelete(nullptr);
}

static void test_freertos_queue_basic() {
    produced = 0;
    consumed = 0;
    q = xQueueCreate(10, sizeof(int));
    TEST_ASSERT_NOT_EQUAL(nullptr, q);

    xTaskCreate(producer_task, "producer", 2048, nullptr, tskIDLE_PRIORITY + 2, nullptr);
    xTaskCreate(consumer_task, "consumer", 2048, nullptr, tskIDLE_PRIORITY + 1, nullptr);

    // Wait until tasks finish (simple timeout guard)
    const uint32_t start = millis();
    while ((consumed < 10 || produced < 10) && (millis() - start) < 3000) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    TEST_ASSERT_EQUAL(10, produced);
    TEST_ASSERT_EQUAL(10, consumed);

    vQueueDelete(q);
}

void setUp() {}
void tearDown() {}

void setup() {
    Serial.begin(9600);
    delay(300);
    UNITY_BEGIN();
    RUN_TEST(test_freertos_queue_basic);
    UNITY_END();
}

void loop() {
    delay(100);
}
