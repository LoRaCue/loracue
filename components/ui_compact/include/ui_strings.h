/**
 * @file ui_strings.h
 * @brief Common UI string constants to reduce binary size
 *
 * Centralizes frequently used UI strings to avoid duplication in binary.
 * Each string is stored once in .rodata and referenced by pointer.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Bottom bar action strings
#define UI_STR_BACK "Back"
#define UI_STR_CANCEL "Cancel"
#define UI_STR_SAVE "Save"
#define UI_STR_EDIT "Edit"
#define UI_STR_MENU "Menu"
#define UI_STR_SELECT "Select"
#define UI_STR_SCROLL "Scroll"
#define UI_STR_YES "Yes"
#define UI_STR_NO "No"
#define UI_STR_ADJUST "Adjust"

// Mode strings
#define UI_STR_PC_MODE "PC MODE"
#define UI_STR_PRESENTER "PRESENTER"

#ifdef __cplusplus
}
#endif
