# One-Button UI Footer Icon Proposal

## Overview

With the one-button UI implementation, we need to update the footer/bottom bar across all screens to show the correct button press gestures using the new icons.

## New Icons Added

Three new icons have been added to `components/oled_ui/icons/ui_status_icons.c`:

1. **Short Press** (7x7): Single circle `○`
2. **Double Press** (13x7): Two circles `○○`
3. **Long Press** (13x7): Circle with hold indicator `●──`

## Current Footer Implementations

### 1. Menu Screen
**Current**: Uses `updown_nav_bits` + "Move" and `both_buttons_bits` + "Select/3s=Exit"

**Proposed**:
```
┌────────────────────────────────┐
│                                │
│        Menu Items              │
│                                │
├────────────────────────────────┤
│ ○ Next  ○○ Prev  ●── Select   │
└────────────────────────────────┘
```

**Implementation**:
```c
// At y=56 (icon position), y=64 (text baseline)
ui_button_short_draw_at(2, 56);
u8g2_DrawStr(&u8g2, 11, 64, "Next");

ui_button_double_draw_at(40, 56);
u8g2_DrawStr(&u8g2, 55, 64, "Prev");

ui_button_long_draw_at(90, 56);
u8g2_DrawStr(&u8g2, 105, 64, "Select");
```

---

### 2. Info Screens (System Info, Device Info, Battery Status)
**Current**: Uses `arrow_prev_bits` + "Back"

**Proposed**:
```
┌────────────────────────────────┐
│                                │
│        Info Content            │
│                                │
├────────────────────────────────┤
│ ●── Back                       │
└────────────────────────────────┘
```

**Implementation**:
```c
// Centered or left-aligned
ui_button_long_draw_at(2, 56);
u8g2_DrawStr(&u8g2, 17, 64, "Back");
```

---

### 3. Selection Screens (Device Mode, Slot, Band, SF, BW, CR, TX Power)
**Current**: Various implementations with up/down arrows

**Proposed**:
```
┌────────────────────────────────┐
│  > Option 1                    │
│    Option 2                    │
│    Option 3                    │
├────────────────────────────────┤
│ ○ Next  ○○ Prev  ●── Select   │
└────────────────────────────────┘
```

**Implementation**: Same as menu screen

---

### 4. Edit Screens (Frequency, Brightness)
**Current**: Various implementations

**Proposed**:
```
┌────────────────────────────────┐
│     Frequency                  │
│      868.1 MHz                 │
│                                │
├────────────────────────────────┤
│ ○ +  ○○ -  ●── Save           │
└────────────────────────────────┘
```

**Implementation**:
```c
ui_button_short_draw_at(2, 56);
u8g2_DrawStr(&u8g2, 11, 64, "+");

ui_button_double_draw_at(30, 56);
u8g2_DrawStr(&u8g2, 45, 64, "-");

ui_button_long_draw_at(70, 56);
u8g2_DrawStr(&u8g2, 85, 64, "Save");
```

---

### 5. Main Screen (Presenter Mode)
**Current**: No footer (full screen for status)

**Proposed**: Keep as-is (no footer needed, button actions are intuitive)

---

### 6. PC Mode Screen
**Current**: No footer

**Proposed**: Keep as-is (displays command history, no navigation needed)

---

### 7. Device Registry Screen
**Current**: Custom footer with navigation hints

**Proposed**:
```
┌────────────────────────────────┐
│  Device 1                      │
│  Device 2                      │
│                                │
├────────────────────────────────┤
│ ○ Next  ○○ Prev  ●── Back     │
└────────────────────────────────┘
```

---

### 8. LoRa Submenu Screen
**Current**: Similar to menu screen

**Proposed**:
```
┌────────────────────────────────┐
│  Presets                       │
│  Frequency                     │
│  Spreading Factor              │
├────────────────────────────────┤
│ ○ Next  ○○ Prev  ●── Select   │
└────────────────────────────────┘
```

---

### 9. Pairing Screen
**Current**: Shows pairing status

**Proposed**:
```
┌────────────────────────────────┐
│    Pairing Mode                │
│   Waiting for PC...            │
│                                │
├────────────────────────────────┤
│ ●── Cancel                     │
└────────────────────────────────┘
```

---

### 10. Factory Reset Screen
**Current**: Confirmation screen

**Proposed**:
```
┌────────────────────────────────┐
│   Factory Reset?               │
│   Hold 5s to confirm           │
│                                │
├────────────────────────────────┤
│ ○○ Cancel  ●── Reset (5s)     │
└────────────────────────────────┘
```

---

## Implementation Strategy

### Phase 1: Create Helper Function
```c
// components/oled_ui/ui_helpers.c

typedef enum {
    FOOTER_CONTEXT_MENU,      // Next, Prev, Select
    FOOTER_CONTEXT_EDIT,      // +, -, Save
    FOOTER_CONTEXT_INFO,      // Back only
    FOOTER_CONTEXT_CONFIRM,   // Cancel, Confirm
    FOOTER_CONTEXT_CUSTOM     // Custom labels
} footer_context_t;

void ui_draw_footer(footer_context_t context, const char *custom_labels[3]) {
    u8g2_DrawHLine(&u8g2, 0, 54, DISPLAY_WIDTH);
    u8g2_SetFont(&u8g2, u8g2_font_helvR08_tr);
    
    switch (context) {
        case FOOTER_CONTEXT_MENU:
            ui_button_short_draw_at(2, 56);
            u8g2_DrawStr(&u8g2, 11, 64, "Next");
            ui_button_double_draw_at(40, 56);
            u8g2_DrawStr(&u8g2, 55, 64, "Prev");
            ui_button_long_draw_at(90, 56);
            u8g2_DrawStr(&u8g2, 105, 64, "Select");
            break;
            
        case FOOTER_CONTEXT_EDIT:
            ui_button_short_draw_at(2, 56);
            u8g2_DrawStr(&u8g2, 11, 64, "+");
            ui_button_double_draw_at(30, 56);
            u8g2_DrawStr(&u8g2, 45, 64, "-");
            ui_button_long_draw_at(70, 56);
            u8g2_DrawStr(&u8g2, 85, 64, "Save");
            break;
            
        case FOOTER_CONTEXT_INFO:
            ui_button_long_draw_at(2, 56);
            u8g2_DrawStr(&u8g2, 17, 64, "Back");
            break;
            
        case FOOTER_CONTEXT_CONFIRM:
            ui_button_double_draw_at(2, 56);
            u8g2_DrawStr(&u8g2, 17, 64, "Cancel");
            ui_button_long_draw_at(70, 56);
            u8g2_DrawStr(&u8g2, 85, 64, "Confirm");
            break;
            
        case FOOTER_CONTEXT_CUSTOM:
            if (custom_labels[0]) {
                ui_button_short_draw_at(2, 56);
                u8g2_DrawStr(&u8g2, 11, 64, custom_labels[0]);
            }
            if (custom_labels[1]) {
                ui_button_double_draw_at(40, 56);
                u8g2_DrawStr(&u8g2, 55, 64, custom_labels[1]);
            }
            if (custom_labels[2]) {
                ui_button_long_draw_at(90, 56);
                u8g2_DrawStr(&u8g2, 105, 64, custom_labels[2]);
            }
            break;
    }
}
```

### Phase 2: Update All Screens
Replace existing footer drawing code with:
```c
// Menu screens
ui_draw_footer(FOOTER_CONTEXT_MENU, NULL);

// Edit screens
ui_draw_footer(FOOTER_CONTEXT_EDIT, NULL);

// Info screens
ui_draw_footer(FOOTER_CONTEXT_INFO, NULL);

// Custom footers
const char *labels[] = {"Next", "Prev", "Back"};
ui_draw_footer(FOOTER_CONTEXT_CUSTOM, labels);
```

---

## Benefits

✅ **Consistent**: Same icon style across all screens  
✅ **Intuitive**: Visual representation of button gestures  
✅ **Compact**: Icons are small (7x7, 13x7), save space  
✅ **Clear**: Users immediately understand interaction method  
✅ **Maintainable**: Centralized footer drawing logic  

---

## Screen-by-Screen Summary

| Screen | Footer Type | Labels |
|--------|-------------|--------|
| Menu | MENU | Next, Prev, Select |
| LoRa Submenu | MENU | Next, Prev, Select |
| Device Mode | MENU | Next, Prev, Select |
| Slot Selection | MENU | Next, Prev, Select |
| Band Selection | MENU | Next, Prev, Select |
| SF Selection | MENU | Next, Prev, Select |
| BW Selection | MENU | Next, Prev, Select |
| CR Selection | MENU | Next, Prev, Select |
| TX Power Selection | MENU | Next, Prev, Select |
| Frequency Edit | EDIT | +, -, Save |
| Brightness Edit | EDIT | +, -, Save |
| System Info | INFO | Back |
| Device Info | INFO | Back |
| Battery Status | INFO | Back |
| Device Registry | CUSTOM | Next, Prev, Back |
| Pairing | INFO | Cancel |
| Factory Reset | CONFIRM | Cancel, Reset (5s) |
| Main Screen | NONE | - |
| PC Mode | NONE | - |

---

## Next Steps

1. ✅ Add icons to `ui_status_icons.c` (DONE)
2. ✅ Add function declarations to `ui_status_icons.h` (DONE)
3. Create `ui_draw_footer()` helper function
4. Update all screens to use new footer
5. Test on hardware
6. Remove old icon definitions from `ui_icons.h` (optional cleanup)

---

## Visual Reference

### Icon Sizes
- Short press: 7x7 pixels
- Double press: 13x7 pixels
- Long press: 13x7 pixels

### Spacing
- Icon at y=56
- Text baseline at y=64
- Horizontal spacing: ~8-10 pixels between icon and text
- Separator line at y=54

### Font
- `u8g2_font_helvR08_tr` (8pt Helvetica)
