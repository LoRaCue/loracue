/**
 * @file ui_helpers.c
 * @brief UI helper functions implementation
 */

#include "ui_helpers.h"

void u8g2_DrawCenterStr(u8g2_t *u8g2, int display_width, int y, const char *str)
{
    int str_width = u8g2_GetStrWidth(u8g2, str);
    int x = (display_width - str_width) / 2;
    u8g2_DrawStr(u8g2, x, y, str);
}
