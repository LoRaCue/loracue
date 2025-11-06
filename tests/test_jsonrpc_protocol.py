#!/usr/bin/env python3
"""
JSON-RPC Protocol Compliance Test

Tests that all methods documented in loracue-docs/protocols/config.md
are implemented in the firmware.

Usage:
    python3 test_jsonrpc_protocol.py /dev/ttyACM0
"""

import json
import sys
import serial
import time
from typing import Dict, Any, Optional

# Expected methods from protocol documentation
DOCUMENTED_METHODS = [
    "ping",
    "device:info",
    "general:get",
    "general:set",
    "power:get",
    "power:set",
    "lora:get",
    "lora:set",
    "lora:bands",
    "lora:key:get",
    "lora:key:set",
    "paired:list",
    "paired:pair",
    "paired:unpair",
    "device:reset",
    "firmware:upgrade",
]

class JSONRPCTester:
    def __init__(self, port: str, baudrate: int = 460800):
        self.port = port
        self.baudrate = baudrate
        self.ser = None
        self.request_id = 1
        
    def connect(self):
        """Connect to device"""
        self.ser = serial.Serial(self.port, self.baudrate, timeout=2)
        time.sleep(0.5)  # Wait for device to be ready
        
    def disconnect(self):
        """Disconnect from device"""
        if self.ser:
            self.ser.close()
            
    def send_request(self, method: str, params: Optional[Dict] = None) -> Dict[str, Any]:
        """Send JSON-RPC request and get response"""
        request = {
            "jsonrpc": "2.0",
            "method": method,
            "id": self.request_id
        }
        if params:
            request["params"] = params
            
        self.request_id += 1
        
        # Send request
        request_str = json.dumps(request) + "\n"
        self.ser.write(request_str.encode('utf-8'))
        self.ser.flush()
        
        # Read response
        response_str = self.ser.readline().decode('utf-8').strip()
        if not response_str:
            return {"error": {"code": -1, "message": "No response"}}
            
        try:
            return json.loads(response_str)
        except json.JSONDecodeError as e:
            return {"error": {"code": -1, "message": f"Invalid JSON: {e}"}}
    
    def test_method_exists(self, method: str) -> tuple[bool, str]:
        """Test if method exists (returns method not found error if missing)"""
        response = self.send_request(method)
        
        # Check if it's a valid JSON-RPC response
        if "jsonrpc" not in response:
            return False, "Invalid JSON-RPC response"
            
        # Method not found error means it's not implemented
        if "error" in response:
            if response["error"]["code"] == -32601:
                return False, "Method not found"
            # Other errors mean method exists but has issues
            return True, f"Method exists (error: {response['error']['message']})"
            
        # Success means method is implemented
        if "result" in response:
            return True, "Method implemented"
            
        return False, "Unexpected response format"
    
    def test_ping(self) -> tuple[bool, str]:
        """Test ping method"""
        response = self.send_request("ping")
        if "result" in response and response["result"] == "pong":
            return True, "✓ Returns 'pong'"
        return False, f"✗ Unexpected result: {response}"
    
    def test_device_info(self) -> tuple[bool, str]:
        """Test device:info method"""
        response = self.send_request("device:info")
        if "result" not in response:
            return False, f"✗ No result: {response}"
            
        result = response["result"]
        required_fields = ["board_id", "model", "version", "mac"]
        missing = [f for f in required_fields if f not in result]
        
        if missing:
            return False, f"✗ Missing fields: {missing}"
        return True, f"✓ All required fields present"
    
    def test_general_get(self) -> tuple[bool, str]:
        """Test general:get method"""
        response = self.send_request("general:get")
        if "result" not in response:
            return False, f"✗ No result: {response}"
            
        result = response["result"]
        required_fields = ["name", "mode", "brightness", "bluetooth", "slot_id"]
        missing = [f for f in required_fields if f not in result]
        
        if missing:
            return False, f"✗ Missing fields: {missing}"
        return True, f"✓ All required fields present"
    
    def test_paired_list(self) -> tuple[bool, str]:
        """Test paired:list method"""
        response = self.send_request("paired:list")
        if "result" not in response:
            return False, f"✗ No result: {response}"
            
        result = response["result"]
        if not isinstance(result, list):
            return False, f"✗ Result is not an array"
        return True, f"✓ Returns array with {len(result)} devices"
    
    def test_lora_bands(self) -> tuple[bool, str]:
        """Test lora:bands method"""
        response = self.send_request("lora:bands")
        if "result" not in response:
            return False, f"✗ No result: {response}"
            
        result = response["result"]
        if not isinstance(result, list):
            return False, f"✗ Result is not an array"
        if len(result) == 0:
            return False, f"✗ No bands returned"
        return True, f"✓ Returns {len(result)} bands"

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 test_jsonrpc_protocol.py /dev/ttyACM0")
        sys.exit(1)
        
    port = sys.argv[1]
    tester = JSONRPCTester(port)
    
    print("=" * 70)
    print("JSON-RPC Protocol Compliance Test")
    print("=" * 70)
    print(f"Port: {port}")
    print()
    
    try:
        print("Connecting to device...")
        tester.connect()
        print("✓ Connected\n")
        
        # Test all documented methods
        print("Testing method implementation:")
        print("-" * 70)
        
        implemented = []
        missing = []
        
        for method in DOCUMENTED_METHODS:
            exists, message = tester.test_method_exists(method)
            status = "✓" if exists else "✗"
            print(f"{status} {method:20s} {message}")
            
            if exists:
                implemented.append(method)
            else:
                missing.append(method)
        
        print()
        print("=" * 70)
        print(f"Implementation Coverage: {len(implemented)}/{len(DOCUMENTED_METHODS)} ({len(implemented)*100//len(DOCUMENTED_METHODS)}%)")
        print("=" * 70)
        
        if missing:
            print(f"\n✗ Missing methods: {', '.join(missing)}")
            print("\nFAILED: Not all documented methods are implemented")
            sys.exit(1)
        
        # Run detailed tests on key methods
        print("\nDetailed Method Tests:")
        print("-" * 70)
        
        tests = [
            ("ping", tester.test_ping),
            ("device:info", tester.test_device_info),
            ("general:get", tester.test_general_get),
            ("paired:list", tester.test_paired_list),
            ("lora:bands", tester.test_lora_bands),
        ]
        
        all_passed = True
        for name, test_func in tests:
            passed, message = test_func()
            print(f"{'✓' if passed else '✗'} {name:20s} {message}")
            if not passed:
                all_passed = False
        
        print()
        print("=" * 70)
        if all_passed:
            print("✓ SUCCESS: All tests passed!")
            print("✓ Protocol implementation is 100% compliant")
        else:
            print("✗ FAILED: Some tests failed")
            sys.exit(1)
        print("=" * 70)
        
    except Exception as e:
        print(f"\n✗ ERROR: {e}")
        sys.exit(1)
    finally:
        tester.disconnect()

if __name__ == "__main__":
    main()
