# Known Issues

> This file tracks known issues in the current release. Update before creating releases.
> Issues listed here will automatically appear in release notes.

## Current Issues

### Implemented But Not Working

- **[HIGH]** BLE configuration connection - Implemented but not working (detection and connect functional)
- **[HIGH]** BLE firmware upgrade - Implemented but not working (OTA engine and BLE transport exist)
- **[HIGH]** USB-CDC firmware upgrade - Implemented but not working (XMODEM-1K and command handler exist)
- **[MEDIUM]** LilyGo T5 Pro support - Implemented but not fully functional (basic firmware works, UX is missing completely, so device is not usable at the moment). This is WIP and will be improved in future releases.

<!-- 
Example format:

- **[CRITICAL]** LoRa transmission fails at SF12 on 433MHz band (#123)
- **[HIGH]** Battery percentage incorrect when charging via USB-C (#124)
- **[MEDIUM]** OLED display flickers during deep sleep wake (#125)
- **[LOW]** LED brightness not adjustable in config mode (#126)

Workarounds:
- For SF12 issues: Use SF11 or switch to 868MHz band
- For battery percentage: Disconnect USB-C for accurate reading
-->
