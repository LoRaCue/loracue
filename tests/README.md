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

Ceedling/Unity/CMock tests that run on the host PC without hardware.

**Framework:**
- Unity (test assertions)
- CMock (auto-generated mocks)
- Ceedling (build system)

**Contents:**
- `test/test_lora_protocol.c` - LoRa protocol unit tests
- `mocks/` - CMock-generated mocks
- `project.yml` - Ceedling configuration

**Run:**
```bash
cd tests/host
bundle install          # First time only
bundle exec ceedling test:all
bundle exec ceedling gcov:all  # With coverage
```

## Coverage

- **JSON-RPC Methods:** 18/18 (100%)
- **Unit Tests:** LoRa integration
- **Host Tests:** LoRa protocol validation
