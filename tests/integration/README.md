# LoRaCue Protocol Tests

## JSON-RPC Protocol Compliance Test

Tests that all methods documented in `loracue-docs/protocols/config.md` are implemented in the firmware.

### Requirements

```bash
pip3 install pyserial
```

### Usage

```bash
# Connect device via USB
python3 test_jsonrpc_protocol.py /dev/ttyACM0
```

### What it tests

1. **Method Coverage** - Verifies all 18 documented methods are implemented:
   - ping
   - device:info
   - general:get/set
   - power:get/set
   - lora:get/set/bands/key:get/key:set/presets:list/presets:set
   - paired:list/pair/unpair
   - device:reset
   - firmware:upgrade

2. **Response Format** - Validates JSON-RPC 2.0 compliance:
   - Correct response structure
   - Proper error codes
   - Required fields present

3. **Method Behavior** - Tests specific method functionality:
   - `ping` returns "pong"
   - `device:info` includes board_id, model, version, mac
   - `general:get` includes name, mode, contrast, bluetooth, slot_id
   - `paired:list` returns array
   - `lora:bands` returns non-empty array

### Expected Output

```
======================================================================
JSON-RPC Protocol Compliance Test
======================================================================
Port: /dev/ttyACM0

Connecting to device...
✓ Connected

Testing method implementation:
----------------------------------------------------------------------
✓ ping                 Method implemented
✓ device:info          Method implemented
✓ general:get          Method implemented
✓ general:set          Method implemented
✓ power:get            Method implemented
✓ power:set            Method implemented
✓ lora:get             Method implemented
✓ lora:set             Method implemented
✓ lora:bands           Method implemented
✓ lora:key:get         Method implemented
✓ lora:key:set         Method implemented
✓ lora:presets:list    Method implemented
✓ lora:presets:set     Method implemented
✓ paired:list          Method implemented
✓ paired:pair          Method implemented
✓ paired:unpair        Method implemented
✓ device:reset         Method implemented
✓ firmware:upgrade     Method implemented

======================================================================
Implementation Coverage: 18/18 (100%)
======================================================================

✓ All documented methods are implemented

PASSED: All documented methods are implemented
```

### Known Issues

- `firmware:upgrade` requires additional parameters (size, sha256, signature) for full test

### Adding New Tests

To add tests for new methods:

1. Add method name to `DOCUMENTED_METHODS` list
2. Create a `test_<method>()` function if detailed testing needed
3. Add to `tests` list in `main()`
