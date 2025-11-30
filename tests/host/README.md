# LoRaCue Host-Based Unit Tests

This directory contains host-based unit tests using the Ceedling/Unity/CMock framework.

## Structure

```
tests/host/
├── test/                       # Test files
│   ├── test_lora_protocol.c   # Unit tests for lora_protocol
│   └── support/               # Test support files
│       ├── esp_log_stub.h     # ESP logging stubs
│       └── freertos_stub.h    # FreeRTOS stubs
├── mocks/                      # Mock headers for CMock
│   ├── mock_lora_driver.h
│   ├── mock_device_registry.h
│   ├── mock_general_config.h
│   ├── mock_power_mgmt.h
│   └── mock_esp_system.h
├── project.yml                 # Ceedling configuration
├── Gemfile                     # Ruby dependencies
└── esp_err.h                   # ESP error code stubs
```

## Running Tests

```bash
cd tests/host
bundle install          # Install dependencies (first time only)
bundle exec ceedling test:all
```

## Test Coverage

```bash
bundle exec ceedling gcov:all
```

Coverage reports are generated in `build/artifacts/gcov/`.

## Writing Tests

1. Create test file: `test/test_<module>.c`
2. Include Unity: `#include "unity.h"`
3. Include module under test: `#include "<module>.h"`
4. Include mocks: `#include "mock_<dependency>.h"`
5. Implement `setUp()` and `tearDown()`
6. Write test functions: `void test_<name>(void)`

### Example

```c
#include "unity.h"
#include "my_module.h"
#include "mock_dependency.h"

void setUp(void) {
    // Initialize before each test
}

void tearDown(void) {
    // Cleanup after each test
}

void test_my_function_returns_success(void) {
    // Arrange
    mock_dependency_init_ExpectAndReturn(ESP_OK);
    
    // Act
    esp_err_t ret = my_function();
    
    // Assert
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}
```

## Best Practices

- ✅ Use Unity assertions (`TEST_ASSERT_*`)
- ✅ Use CMock for dependencies (`*_ExpectAndReturn`, `*_Stub`)
- ✅ Keep tests focused and isolated
- ✅ Use `setUp()`/`tearDown()` for common initialization
- ✅ Test one behavior per test function
- ✅ Use descriptive test names
- ❌ Don't include `.c` files directly
- ❌ Don't write manual mocks
- ❌ Don't test implementation details
