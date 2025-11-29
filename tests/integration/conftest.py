import pytest
import serial
import time
import json
import sys

def pytest_addoption(parser):
    parser.addoption("--port", action="store", default="/dev/ttyACM0", help="Serial port to connect to")
    parser.addoption("--baud", action="store", default=460800, type=int, help="Baud rate")

@pytest.fixture(scope="session")
def serial_port(request):
    port = request.config.getoption("--port")
    baud = request.config.getoption("--baud")
    
    try:
        ser = serial.Serial(port, baud, timeout=2)
        time.sleep(0.5) # Wait for device
        yield ser
        ser.close()
    except serial.SerialException as e:
        pytest.fail(f"Could not open serial port {port}: {e}")

@pytest.fixture
def rpc_client(serial_port):
    """Returns a callable that sends a JSON-RPC request and returns the response"""
    request_id = 1
    
    def _send(method, params=None):
        nonlocal request_id
        req = {
            "jsonrpc": "2.0",
            "method": method,
            "id": request_id
        }
        if params:
            req["params"] = params
            
        request_id += 1
        
        # Clear input buffer
        serial_port.reset_input_buffer()
        
        # Send
        req_str = json.dumps(req) + "\n"
        serial_port.write(req_str.encode('utf-8'))
        serial_port.flush()
        
        # Read
        try:
            line = serial_port.readline().decode('utf-8').strip()
            if not line:
                return {"error": {"code": -1, "message": "No response"}}
            return json.loads(line)
        except json.JSONDecodeError as e:
            return {"error": {"code": -1, "message": f"Invalid JSON: {e}"}}
            
    return _send
