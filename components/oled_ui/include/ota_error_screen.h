#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Draw OTA compatibility error screen
 * 
 * @param current_board Current board ID
 * @param new_board New firmware board ID
 */
void ota_error_screen_draw(const char *current_board, const char *new_board);

#ifdef __cplusplus
}
#endif
