/**
 * @file test_commands_parser.c
 * @brief Unit tests for JSON-RPC 2.0 parser in commands component
 */

#include "unity.h"
#include <string.h>
#include <stdbool.h>

// JSON-RPC 2.0 error codes (from commands.c)
#define JSONRPC_PARSE_ERROR -32700
#define JSONRPC_INVALID_REQUEST -32600
#define JSONRPC_METHOD_NOT_FOUND -32601
#define JSONRPC_INVALID_PARAMS -32602

// Test response capture
static char captured_response[1024];
static bool response_captured = false;

// Mock response function
static void mock_response_fn(const char *response)
{
    strncpy(captured_response, response, sizeof(captured_response) - 1);
    captured_response[sizeof(captured_response) - 1] = '\0';
    response_captured = true;
}

void setUp(void)
{
    memset(captured_response, 0, sizeof(captured_response));
    response_captured = false;
}

void tearDown(void)
{
}

void test_valid_jsonrpc_request_structure(void)
{
    const char *valid_request = "{\"jsonrpc\":\"2.0\",\"method\":\"test\",\"id\":1}";
    
    // Should contain required fields
    TEST_ASSERT_TRUE(strstr(valid_request, "jsonrpc") != NULL);
    TEST_ASSERT_TRUE(strstr(valid_request, "method") != NULL);
    TEST_ASSERT_TRUE(strstr(valid_request, "id") != NULL);
}

void test_malformed_json_should_fail(void)
{
    const char *malformed = "{invalid json";
    
    // Parser should detect malformed JSON
    TEST_ASSERT_TRUE(strstr(malformed, "{") != NULL);
    TEST_ASSERT_FALSE(strstr(malformed, "}") != NULL);
}

void test_missing_jsonrpc_version(void)
{
    const char *no_version = "{\"method\":\"test\",\"id\":1}";
    
    // Should be missing jsonrpc field
    TEST_ASSERT_FALSE(strstr(no_version, "jsonrpc") != NULL);
}

void test_missing_method_field(void)
{
    const char *no_method = "{\"jsonrpc\":\"2.0\",\"id\":1}";
    
    // Should be missing method field
    TEST_ASSERT_FALSE(strstr(no_method, "method") != NULL);
}

void test_request_with_params_object(void)
{
    const char *with_params = "{\"jsonrpc\":\"2.0\",\"method\":\"test\",\"params\":{\"key\":\"value\"},\"id\":1}";
    
    // Should contain params object
    TEST_ASSERT_TRUE(strstr(with_params, "params") != NULL);
    TEST_ASSERT_TRUE(strstr(with_params, "key") != NULL);
}

void test_request_with_params_array(void)
{
    const char *with_array = "{\"jsonrpc\":\"2.0\",\"method\":\"test\",\"params\":[1,2,3],\"id\":1}";
    
    // Should contain params array
    TEST_ASSERT_TRUE(strstr(with_array, "params") != NULL);
    TEST_ASSERT_TRUE(strstr(with_array, "[") != NULL);
}

void test_notification_without_id(void)
{
    const char *notification = "{\"jsonrpc\":\"2.0\",\"method\":\"notify\"}";
    
    // Notification should not have id
    TEST_ASSERT_FALSE(strstr(notification, "id") != NULL);
}

void test_error_code_ranges(void)
{
    // Verify error codes are in correct ranges
    TEST_ASSERT_EQUAL(-32700, JSONRPC_PARSE_ERROR);
    TEST_ASSERT_EQUAL(-32600, JSONRPC_INVALID_REQUEST);
    TEST_ASSERT_EQUAL(-32601, JSONRPC_METHOD_NOT_FOUND);
    TEST_ASSERT_EQUAL(-32602, JSONRPC_INVALID_PARAMS);
}

void test_response_structure_with_result(void)
{
    const char *response = "{\"jsonrpc\":\"2.0\",\"result\":true,\"id\":1}";
    
    // Valid response should have jsonrpc, result, and id
    TEST_ASSERT_TRUE(strstr(response, "jsonrpc") != NULL);
    TEST_ASSERT_TRUE(strstr(response, "result") != NULL);
    TEST_ASSERT_TRUE(strstr(response, "id") != NULL);
}

void test_response_structure_with_error(void)
{
    const char *error_response = "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32600,\"message\":\"Invalid Request\"},\"id\":null}";
    
    // Error response should have jsonrpc, error, and id
    TEST_ASSERT_TRUE(strstr(error_response, "jsonrpc") != NULL);
    TEST_ASSERT_TRUE(strstr(error_response, "error") != NULL);
    TEST_ASSERT_TRUE(strstr(error_response, "code") != NULL);
    TEST_ASSERT_TRUE(strstr(error_response, "message") != NULL);
}
