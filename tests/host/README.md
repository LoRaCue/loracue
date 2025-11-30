# LoRaCue Host-Based Unit Tests

This directory contains host-based unit tests using the Ceedling/Unity framework.

## Structure

```
tests/host/
├── test/                       # Test files
│   ├── test_mock_setup.c      # Framework verification tests
│   └── support/               # Test support files
│       ├── esp_log_stub.h     # ESP logging stubs
│       └── freertos_stub.h    # FreeRTOS stubs
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

## Writing Tests

1. Create test file: `test/test_<module>.c`
2. Include Unity: `#include "unity.h"`
3. Implement `setUp()` and `tearDown()`
4. Write test functions: `void test_<name>(void)`

### Example

```c
#include "unity.h"
#include <stdbool.h>

void setUp(void) {
    // Initialize before each test
}

void tearDown(void) {
    // Cleanup after each test
}

void test_my_function_returns_success(void) {
    // Arrange
    int expected = 42;
    
    // Act
    int result = my_function();
    
    // Assert
    TEST_ASSERT_EQUAL(expected, result);
}
```

## Best Practices

- ✅ Use Unity assertions (`TEST_ASSERT_*`)
- ✅ Keep tests focused and isolated
- ✅ Use `setUp()`/`tearDown()` for common initialization
- ✅ Test one behavior per test function
- ✅ Use descriptive test names
- ❌ Don't test implementation details
