# UX Architecture - Dual UI System

## Philosophy

LoRaCue supports two fundamentally different UX paradigms that should **not** be forced into a shared abstraction:

1. **Embedded UI**: Small displays, button-only navigation
2. **Rich UI**: Large displays, touch/advanced input

## Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│                  Shared Business Logic                  │
│  device_config, lora_protocol, device_registry, etc.    │
└────────────────┬────────────────────────────────────────┘
                 │
        ┌────────┴────────┐
        ↓                 ↓
┌───────────────┐  ┌──────────────┐
│  Embedded UI  │  │   Rich UI    │
│               │  │              │
│ • u8g2        │  │ • LVGL       │
│ • <200px      │  │ • >240px     │
│ • Buttons     │  │ • Touch      │
│ • Sequential  │  │ • Dashboard  │
└───────────────┘  └──────────────┘
```

## 1. Embedded UI System

**Target Hardware:**
- Heltec LoRa V3 (128x64 OLED, 1 button)
- Custom boards with small monochrome displays
- Button-only or rotary encoder input

**Framework:** u8g2 (lightweight, perfect for embedded)

**UX Paradigm:**
- Hierarchical menu navigation
- Modal dialogs and editors
- Sequential, linear flow
- One screen at a time
- Text-heavy, minimal graphics

**Directory Structure:**
```
components/embedded_ui/
├── include/
│   ├── embedded_ui.h
│   └── embedded_screens.h
├── embedded_ui.c
├── embedded_screen_controller.c
├── screens/
│   ├── menu_screen.c
│   ├── value_editor_screen.c
│   ├── list_selector_screen.c
│   └── status_screen.c
└── CMakeLists.txt
```

**Example Screen:**
```
┌────────────────┐
│ ▶ Device Mode  │  4 lines visible
│   Slot         │  Sequential nav
│   LoRa Config  │  8px font
│   Brightness   │  Text-only
└────────────────┘
```

## 2. Rich UI System

**Target Hardware:**
- Custom boards with color TFT displays (320x240+)
- Touchscreen support
- Multiple buttons or rotary encoders

**Framework:** LVGL (widgets, animations, themes)

**UX Paradigm:**
- Dashboard with multiple info zones
- Direct manipulation (tap, swipe, drag)
- Parallel information display
- Rich graphics, icons, charts
- Animations and visual feedback

**Directory Structure:**
```
components/rich_ui/
├── include/
│   ├── rich_ui.h
│   └── rich_screens.h
├── rich_ui.c
├── rich_screen_controller.c
├── screens/
│   ├── dashboard_screen.c
│   ├── settings_panel_screen.c
│   ├── graph_view_screen.c
│   └── device_list_screen.c
└── CMakeLists.txt
```

**Example Screen:**
```
┌────────────────────────────────┐
│ ┌─────┐ ┌─────┐ ┌─────┐       │
│ │ 85% │ │ LoRa│ │ PRE │       │  Status cards
│ │ 🔋  │ │ ●●● │ │ MODE│       │
│ └─────┘ └─────┘ └─────┘       │
│                                │
│ ┌──────────┐ ┌──────────┐     │  Quick actions
│ │   NEXT   │ │   PREV   │     │  (touch buttons)
│ │  SLIDE   │ │  SLIDE   │     │
│ └──────────┘ └──────────┘     │
│                                │
│ Recent Activity:               │  Activity graph
│ ▁▂▃▅▇█▇▅▃▂▁                   │
└────────────────────────────────┘
```

## 3. Shared Business Logic

Both UI systems call the same underlying APIs:

```c
// Device configuration
device_config_get(&config);
device_config_set_mode(DEVICE_MODE_PRESENTER);
device_config_set_slot(3);

// LoRa protocol
lora_protocol_send_keyboard_reliable(slot_id, modifiers, keycode, timeout, retries);

// Device registry
device_registry_add_device(&device_info);
device_registry_get_paired_count();
```

**Shared Components:**
- `components/device_config/`
- `components/lora_protocol/`
- `components/device_registry/`
- `components/power_mgmt/`
- `components/button_manager/` (input abstraction)
- `components/input_manager/` (future: unified input events)

## 4. Configuration Selection

**Kconfig (mutually exclusive):**
```kconfig
choice LORACUE_UI_SYSTEM
    prompt "User Interface System"
    default UI_EMBEDDED
    
config UI_EMBEDDED
    bool "Embedded UI (small OLED, buttons)"
    select U8G2
    help
        Lightweight UI for small monochrome displays with button-only navigation.
        Optimized for low memory and simple input devices.
    
config UI_RICH
    bool "Rich UI (large TFT, touch)"
    select LVGL
    help
        Feature-rich UI for color displays with touch support.
        Includes dashboard views, animations, and advanced widgets.
endchoice
```

**BSP Selection:**
```c
// bsp_heltec_v3.c
#define LORACUE_UI_EMBEDDED

// bsp_custom_tft.c
#define LORACUE_UI_RICH
```

## 5. Input Abstraction (Shared)

While UI systems are separate, input can be abstracted:

```c
// components/input_manager/input_manager.h

typedef enum {
    INPUT_EVENT_NAVIGATE_NEXT,
    INPUT_EVENT_NAVIGATE_PREV,
    INPUT_EVENT_SELECT,
    INPUT_EVENT_CANCEL,
    INPUT_EVENT_INCREMENT,
    INPUT_EVENT_DECREMENT,
    INPUT_EVENT_ABSOLUTE_POSITION  // For touch/rotary
} input_event_type_t;

typedef struct {
    input_event_type_t type;
    int32_t value;  // For absolute position, scroll delta, etc.
    void *context;
} input_event_t;
```

**Input Backends:**
- `one_button_backend.c`: Short→NEXT, Double→PREV, Long→SELECT
- `multi_button_backend.c`: Dedicated buttons for each action
- `rotary_encoder_backend.c`: Rotation→NEXT/PREV, Press→SELECT
- `touchscreen_backend.c`: Tap→SELECT, Swipe→NEXT/PREV

## 6. Display Capabilities (Per UI System)

```c
// Embedded UI capabilities
typedef struct {
    uint16_t width;           // 128
    uint16_t height;          // 64
    uint8_t lines_visible;    // 4
    uint8_t line_height;      // 16
    bool monochrome;          // true
} embedded_display_t;

// Rich UI capabilities
typedef struct {
    uint16_t width;           // 320
    uint16_t height;          // 240
    uint8_t color_depth;      // 16-bit
    bool touch_support;       // true
    bool animation_support;   // true
} rich_display_t;
```

## 7. Migration Path

### Current State (v1.0)
- `components/oled_ui/` → Rename to `components/embedded_ui/`
- Keep u8g2-based implementation
- One-button navigation model

### Future (v2.0)
- Add `components/rich_ui/` when building touch hardware
- LVGL-based dashboard implementation
- Touch gesture support

### Build System
```cmake
# Only one UI system compiled per build
if(CONFIG_UI_EMBEDDED)
    list(APPEND EXTRA_COMPONENT_DIRS "components/embedded_ui")
elseif(CONFIG_UI_RICH)
    list(APPEND EXTRA_COMPONENT_DIRS "components/rich_ui")
endif()
```

## 8. Benefits of Dual System Approach

✅ **No forced abstraction**: Each UI uses its framework's strengths  
✅ **Optimal UX**: Button UI feels snappy, touch UI feels modern  
✅ **Simpler code**: No translation layers, direct framework usage  
✅ **Independent evolution**: Add LVGL features without affecting embedded  
✅ **Clear separation**: Developer knows which UI they're working on  
✅ **Memory efficient**: Only one UI system compiled per build  
✅ **Framework-native**: Use u8g2/LVGL idioms directly  

## 9. Anti-Patterns to Avoid

❌ **Shared screen rendering**: Don't try to make menu_screen.c work for both  
❌ **Display abstraction layer**: Don't wrap u8g2/LVGL in common API  
❌ **Runtime UI switching**: Don't compile both UIs and switch at runtime  
❌ **Lowest common denominator**: Don't limit rich UI to embedded capabilities  

## 10. When to Share Code

✅ **Business logic**: Device config, LoRa protocol, registry  
✅ **Input events**: Abstract button/touch/rotary to common events  
✅ **Data structures**: `device_status_t`, `lora_config_t`, etc.  
✅ **Utilities**: String formatting, validation, calculations  

❌ **Screen rendering**: Keep separate per UI system  
❌ **Navigation flow**: Different paradigms (sequential vs. dashboard)  
❌ **Visual design**: Different constraints (text vs. graphics)  

## Conclusion

By maintaining two distinct UI systems, LoRaCue can provide optimal user experiences for both embedded devices (small displays, buttons) and rich devices (large displays, touch) without compromising either. The shared business logic ensures consistency while allowing each UI to leverage its framework's strengths.
