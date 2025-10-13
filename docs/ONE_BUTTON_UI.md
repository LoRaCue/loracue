# One-Button UI Design

## Hardware Constraint
Heltec LoRa V3 has only **one user button** (GPIO 0). The second button is hardwired to RESET.

## Button Actions

| Action | Timing | Function |
|--------|--------|----------|
| **Short Press** | <500ms | Next / Navigate Down / Increment |
| **Double Press** | 2 clicks <500ms apart | Back / Cancel / Navigate Up |
| **Long Press** | >3s | Enter Menu / Confirm / Save |

## Navigation Model

### Main Screen (Presenter/PC Mode)
- **Short**: Cycle through info screens (battery, signal, etc.)
- **Double**: *(unused)*
- **Long**: Enter main menu

### Menu Screen
- **Short**: Navigate down (next item)
- **Double**: Navigate up (previous item)
- **Long**: Exit menu (return to main screen)

### Selection Screen (e.g., Device Mode, Band, Slot)
- **Short**: Navigate down (next option)
- **Double**: Navigate up (previous option)
- **Long**: Select and return to parent menu

### Edit Screen (e.g., Frequency, TX Power, Brightness)
- **Short**: Increment value
- **Double**: Decrement value
- **Long**: Save and return to parent menu

### Submenu Screen (e.g., LoRa Settings)
- **Short**: Navigate down (next item)
- **Double**: Navigate up (previous item)
- **Long**: Return to parent menu

## Screen Footer Design

All screens show single-button hints:

```
┌────────────────────────────────┐
│                                │
│        Screen Content          │
│                                │
├────────────────────────────────┤
│ ○ Next  ○○ Back  ●── Menu     │
└────────────────────────────────┘
```

**Icons:**
- `○` = Short press
- `○○` = Double press
- `●──` = Long press (hold)

**Context-specific labels:**
- Main: `○ Info  ●── Menu`
- Menu: `○ Next  ○○ Prev  ●── Exit`
- Select: `○ Next  ○○ Prev  ●── Select`
- Edit: `○ +  ○○ -  ●── Save`

## Implementation Changes

### 1. Button Manager
```c
typedef enum {
    BUTTON_EVENT_SHORT,        // <500ms
    BUTTON_EVENT_DOUBLE,       // 2 clicks <500ms apart
    BUTTON_EVENT_LONG          // >3s
} button_event_type_t;
```

### 2. Screen Interface
```c
typedef struct {
    void (*init)(void);
    void (*draw)(void);
    void (*on_short)(void);    // Short press handler
    void (*on_double)(void);   // Double press handler
    void (*on_long)(void);     // Long press handler
} screen_interface_t;
```

### 3. Screen Controller
- Remove `oled_button_t` enum (PREV/NEXT/BOTH)
- Replace with single button event routing
- Each screen implements 3 handlers

### 4. Footer Helper
```c
void ui_draw_footer(const char *short_label, const char *double_label, const char *long_label);
```

## Migration Strategy

1. Update button_manager to detect short/double/long
2. Create new screen interface structure
3. Update screen_controller to route single button events
4. Create footer drawing helper
5. Migrate screens one by one:
   - Main screen
   - Menu screen
   - Selection screens (mode, band, slot)
   - Edit screens (frequency, power, brightness)
   - Submenu screens (LoRa settings)

## Benefits

✅ **Hardware accurate**: Matches Heltec V3 single button  
✅ **Extensible**: Easy to add new screens with 3 handlers  
✅ **Consistent**: Same navigation pattern everywhere  
✅ **Intuitive**: Short=next, Double=back, Long=action  
✅ **Clean code**: Screen interface abstraction  

## Presenter Mode Behavior

In presenter mode (not in menu):
- **Short**: Send NEXT slide command
- **Double**: Send PREV slide command
- **Long**: Enter menu (no slide command)

This preserves the core presentation functionality while adding menu access.
