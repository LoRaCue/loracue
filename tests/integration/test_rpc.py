import pytest

DOCUMENTED_METHODS = [
    "ping", "device:info", "general:get", "general:set",
    "power:get", "power:set", "lora:get", "lora:set",
    "lora:bands", "lora:key:get", "lora:key:set",
    "lora:presets:list", "lora:presets:set",
    "paired:list", "paired:pair", "paired:unpair",
    "device:reset", "firmware:upgrade",
]

@pytest.mark.parametrize("method", DOCUMENTED_METHODS)
def test_method_exists(rpc_client, method):
    """Verify that all documented methods exist (don't return Method Not Found)"""
    resp = rpc_client(method)
    
    assert "jsonrpc" in resp, "Invalid JSON-RPC response"
    
    if "error" in resp:
        # -32601 is Method not found
        assert resp["error"]["code"] != -32601, f"Method {method} not implemented"

def test_ping(rpc_client):
    resp = rpc_client("ping")
    assert resp.get("result") == "pong"

def test_device_info(rpc_client):
    resp = rpc_client("device:info")
    assert "result" in resp
    res = resp["result"]
    assert "board_id" in res
    assert "model" in res
    assert "version" in res
    assert "mac" in res

def test_general_config(rpc_client):
    # Get current
    resp = rpc_client("general:get")
    assert "result" in resp
    original = resp["result"]
    
    # Verify fields
    required = ["name", "mode", "contrast", "bluetooth", "slot_id"]
    for field in required:
        assert field in original
        
    # Try setting a value (contrast)
    new_contrast = 100 if original["contrast"] != 100 else 200
    
    # Copy original and modify
    new_config = original.copy()
    new_config["contrast"] = new_contrast
    
    # Set
    resp = rpc_client("general:set", new_config)
    assert resp.get("result") == "OK"
    
    # Verify set
    resp = rpc_client("general:get")
    assert resp["result"]["contrast"] == new_contrast
    
    # Restore
    rpc_client("general:set", original)

def test_lora_bands(rpc_client):
    resp = rpc_client("lora:bands")
    assert "result" in resp
    assert isinstance(resp["result"], list)
    assert len(resp["result"]) > 0

def test_paired_list(rpc_client):
    resp = rpc_client("paired:list")
    assert "result" in resp
    assert isinstance(resp["result"], list)
