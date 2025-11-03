# Rust BLE OTA Quick Start

## TL;DR

Implement BLE OTA in LoRaCueManager (Rust) using:
- **btleplug** for BLE
- **prost** for Protocol Buffers
- **tokio** for async

Reference Java implementation: https://github.com/EspressifApps/esp-ble-ota-android

---

## Minimal Dependencies

```toml
[dependencies]
btleplug = "0.11"
prost = "0.12"
tokio = { version = "1.35", features = ["full"] }
uuid = "1.6"
thiserror = "1.0"

[build-dependencies]
prost-build = "0.12"
```

---

## Protocol Files

Copy from firmware:
```bash
cp ~/Source/LoRaCue/components/ble_ota/proto/*.proto ./proto/
```

Generate Rust code in `build.rs`:
```rust
fn main() {
    prost_build::compile_protos(
        &["proto/ota_send_file.proto", "proto/constants.proto"],
        &["proto/"],
    ).unwrap();
}
```

---

## BLE Service UUIDs

```rust
const OTA_SERVICE: Uuid = uuid::uuid!("00008018-0000-1000-8000-00805f9b34fb");
const RECV_FW: Uuid = uuid::uuid!("00008020-0000-1000-8000-00805f9b34fb");
const OTA_STATUS: Uuid = uuid::uuid!("00008021-0000-1000-8000-00805f9b34fb");
const COMMAND: Uuid = uuid::uuid!("00008022-0000-1000-8000-00805f9b34fb");
```

---

## OTA Flow

```rust
// 1. Connect
let peripheral = connect_device(&adapter, "LoRaCue-XXXX").await?;

// 2. Start OTA
let start = CmdStartOta {
    file_size: firmware.len() as u64,
    block_size: 512,
    partition_name: "ota_0".into(),
};
write_protobuf(&peripheral, &cmd_char, start).await?;

// 3. Send blocks
for chunk in firmware.chunks(512) {
    let send = CmdSendFile { data: chunk.to_vec() };
    write_protobuf(&peripheral, &recv_fw_char, send).await?;
}

// 4. Finish
let finish = CmdFinishOta {};
write_protobuf(&peripheral, &cmd_char, finish).await?;
```

---

## Key Functions

### Connect
```rust
async fn connect_device(adapter: &Adapter, name: &str) -> Result<Peripheral> {
    adapter.start_scan(ScanFilter::default()).await?;
    tokio::time::sleep(Duration::from_secs(5)).await;
    
    for p in adapter.peripherals().await? {
        if p.properties().await?.local_name.as_deref() == Some(name) {
            p.connect().await?;
            p.discover_services().await?;
            return Ok(p);
        }
    }
    Err(OtaError::DeviceNotFound)
}
```

### Write Protobuf
```rust
async fn write_protobuf<M: prost::Message>(
    peripheral: &Peripheral,
    char: &Characteristic,
    msg: M,
) -> Result<()> {
    let mut buf = Vec::new();
    msg.encode(&mut buf)?;
    peripheral.write(char, &buf, WriteType::WithResponse).await?;
    Ok(())
}
```

---

## Security

Device requires bonding before OTA:
- First connection: OS shows PIN dialog
- User enters 6-digit PIN from device OLED
- Subsequent connections: automatic (bonded)

---

## Full Documentation

See: `~/Source/LoRaCue/docs/LORACUE_MANAGER_BLE_OTA_PROMPT.md`
