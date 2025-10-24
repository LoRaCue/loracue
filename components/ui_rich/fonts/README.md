# SF Pro Fonts for LVGL

This directory contains auto-generated LVGL font files from SF Pro.

## Build-Time Generation

Fonts are automatically generated during the build process from the SF Pro submodule at `assets/fonts/sf-pro`.

### Prerequisites

```bash
npm install -g lv_font_conv
```

### Generated Fonts

- `sf_pro_16.c` - SF Pro Display Regular 16px
- `sf_pro_24.c` - SF Pro Display Regular 24px  
- `sf_pro_bold_24.c` - SF Pro Display Bold 24px

### Manual Generation

To regenerate fonts manually:

```bash
cd components/ui_rich
./generate_fonts.sh
```

### Font Configuration

The generation script (`generate_fonts.sh`) uses:
- Character range: 0x20-0x7E (ASCII printable)
- BPP: 4 (16 grayscale levels)
- Format: LVGL
- No compression (for faster rendering on E-Paper)

### Usage in Code

```c
// Declare font
LV_FONT_DECLARE(sf_pro_24);

// Use font
lv_obj_set_style_text_font(label, &sf_pro_24, 0);
```
