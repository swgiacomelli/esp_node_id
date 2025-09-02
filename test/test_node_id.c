
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "node_id.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Unity setup/teardown
void setUp(void) {}
void tearDown(void) {}

// Test: Default node ID (MAC-based)
void test_default_node_id(void)
{
    char *id = NULL;
    size_t len = 0;
    TEST_ASSERT_EQUAL(ERROR_OK, get_node_id(&id, &len));
    TEST_ASSERT_NOT_NULL(id);
    TEST_ASSERT_TRUE(len > 0);
    TEST_ASSERT_TRUE(strlen(id) == len);
}

// Test: Custom identity
void test_custom_identity(void)
{
    const uint8_t custom_id[] = "TEST_CUSTOM_IDENTITY";
    TEST_ASSERT_EQUAL(ERROR_OK, node_id_force_init(custom_id, sizeof(custom_id) - 1));
    char *id = NULL;
    size_t len = 0;
    TEST_ASSERT_EQUAL(ERROR_OK, get_node_id(&id, &len));
    TEST_ASSERT_NOT_NULL(id);
    // Node ID should change after force init
}

// Thread safety helpers
static void thread_init_task(void *param)
{
    const uint8_t *identity = (const uint8_t *)param;
    for (int i = 0; i < 10; ++i)
    {
        TEST_ASSERT_EQUAL(ERROR_OK, node_id_init(identity, strlen((const char *)identity)));
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}

static void thread_get_task(void *param)
{
    for (int i = 0; i < 10; ++i)
    {
        char *id = NULL;
        size_t len = 0;
        TEST_ASSERT_EQUAL(ERROR_OK, get_node_id(&id, &len));
        TEST_ASSERT_NOT_NULL(id);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}

// Test: Thread safety
void test_thread_safety(void)
{
    const uint8_t id1[] = "THREAD_ID1";
    const uint8_t id2[] = "THREAD_ID2";
    xTaskCreate(thread_init_task, "init_task1", 2048, (void *)id1, 5, NULL);
    xTaskCreate(thread_init_task, "init_task2", 2048, (void *)id2, 5, NULL);
    xTaskCreate(thread_get_task, "get_task1", 2048, NULL, 5, NULL);
    xTaskCreate(thread_get_task, "get_task2", 2048, NULL, 5, NULL);
    vTaskDelay(pdMS_TO_TICKS(300));
}

// Unity test cases
TEST_CASE("Default node ID", "[node_id]") { test_default_node_id(); }
TEST_CASE("Custom identity", "[node_id]") { test_custom_identity(); }
TEST_CASE("Thread safety", "[node_id]") { test_thread_safety(); }

// Main entry
int app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_default_node_id);
    RUN_TEST(test_custom_identity);
    RUN_TEST(test_thread_safety);
    return UNITY_END();
}
