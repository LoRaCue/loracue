/**
 * @file test_mock_setup.c
 * @brief Test to verify Ceedling/Unity/CMock setup is working
 */

#include "unity.h"
#include <stdbool.h>

void setUp(void)
{
    // Setup before each test
}

void tearDown(void)
{
    // Cleanup after each test
}

void test_unity_framework_works(void)
{
    TEST_ASSERT_EQUAL(1, 1);
    TEST_ASSERT_TRUE(true);
    TEST_ASSERT_FALSE(false);
}

void test_basic_arithmetic(void)
{
    int result = 2 + 2;
    TEST_ASSERT_EQUAL(4, result);
}

void test_string_comparison(void)
{
    const char *str = "LoRaCue";
    TEST_ASSERT_EQUAL_STRING("LoRaCue", str);
}
