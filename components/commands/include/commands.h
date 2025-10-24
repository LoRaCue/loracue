#ifndef COMMANDS_H
#define COMMANDS_H

#include <stddef.h>

/**
 * @brief Response callback function type
 *
 * @param response The response string to send back to the client
 */
typedef void (*response_fn_t)(const char *response);

/**
 * @brief Execute a command string
 *
 * @param command_line The command string to execute
 * @param send_response Callback function to send responses
 */
void commands_execute(const char *command_line, response_fn_t send_response);

#endif // COMMANDS_H
