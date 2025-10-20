# Known Issues

> This file tracks known issues in the current release. Update before creating releases.
> Issues listed here will automatically appear in release notes.

## Current Issues

### Not Yet Implemented

- **[HIGH]** BLE configuration connection - Detection and connect working, but configuration protocol not implemented
- **[HIGH]** BLE firmware upgrade - OTA engine exists but BLE transport not implemented
- **[HIGH]** USB-CDC firmware upgrade - FIRMWARE_UPGRADE command not implemented in command parser

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
