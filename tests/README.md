# LoRaCue Test Suite

Comprehensive testing infrastructure for LoRaCue firmware.

## Directory Structure

```
tests/
├── unit/           # ESP-IDF C unit tests (on-device)
├── integration/    # Python protocol tests (external)
└── host/           # Host-based tests (PC)
```

## Unit Tests (`unit/`)

ESP-IDF Unity-based tests that run on actual hardware or simulator.

**Contents:**
- `test_lora_integration.c` - LoRa driver integration tests

**Run:**
```bash
make test  # Run on connected device
```

## Integration Tests (`integration/`)

Python-based protocol compliance tests via serial port.

**Contents:**
- `test_jsonrpc_protocol.py` - JSON-RPC 2.0 API compliance

**Run:**
```bash
cd tests/integration
pip3 install pyserial
python3 test_jsonrpc_protocol.py /dev/ttyACM0
```

## Host Tests (`host/`)

Tests that run on the host PC without hardware.

**Contents:**
- `test_lora_host.c` - LoRa protocol tests with mocked hardware

**Run:**
```bash
cd tests/host
gcc -o test_lora_host test_lora_host.c
./test_lora_host
```

## Coverage

- **JSON-RPC Methods:** 18/18 (100%)
- **Unit Tests:** LoRa integration
- **Host Tests:** LoRa protocol validation
