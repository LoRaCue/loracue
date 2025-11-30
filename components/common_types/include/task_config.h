/**
 * @file task_config.h
 * @brief Common task configuration constants for LoRaCue
 *
 * Standardized task stack sizes and priorities to ensure consistent
 * resource allocation across all FreeRTOS tasks.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Standard task stack sizes (in bytes)
 *
 * These values are based on actual usage patterns and stack overflow analysis.
 * Sizes are conservative to prevent stack overflow while minimizing RAM usage.
 */
#define TASK_STACK_SIZE_SMALL 2048  ///< Simple state machines, minimal local variables
#define TASK_STACK_SIZE_MEDIUM 3072 ///< Network/protocol tasks, moderate complexity
#define TASK_STACK_SIZE_LARGE 4096  ///< UI tasks, complex logic, deep call stacks

/**
 * @brief Standard task priorities
 *
 * FreeRTOS priority levels (0 = lowest, configMAX_PRIORITIES-1 = highest)
 * ESP-IDF typically uses priorities 1-24, with system tasks at 25+
 */
#define TASK_PRIORITY_LOW 3    ///< Background tasks, non-critical
#define TASK_PRIORITY_NORMAL 5 ///< Standard application tasks
#define TASK_PRIORITY_HIGH 7   ///< Time-sensitive tasks (LoRa, USB)

#ifdef __cplusplus
}
#endif
