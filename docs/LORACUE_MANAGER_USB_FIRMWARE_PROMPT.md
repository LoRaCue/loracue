# LoRaCueManager USB CDC Firmware Upload Implementation

## Objective

Implement simple binary firmware upload via USB CDC in LoRaCueManager (Rust).

---

## Protocol (Simple Binary Upload)

### Step 1: Send Command
```
FIRMWARE_UPGRADE <size>\n
```

### Step 2: Wait for Response
```
OK Ready\n
```

### Step 3: Send Raw Binary
```
[<size> bytes of firmware data]
```

### Step 4: Wait for Completion
```
OK Complete, rebooting...\n
```

**That's it. No XMODEM, no protocol overhead, just raw binary.**

---

## Rust Implementation

### Dependencies

```toml
[dependencies]
serialport = "4.3"
```

### Code

```rust
use serialport::SerialPort;
use std::io::{Read, Write};
use std::time::Duration;

pub fn upload_firmware(
    port: &mut Box<dyn SerialPort>,
    firmware: &[u8],
) -> Result<(), Box<dyn std::error::Error>> {
    // Step 1: Send command
    let cmd = format!("FIRMWARE_UPGRADE {}\n", firmware.len());
    port.write_all(cmd.as_bytes())?;
    port.flush()?;
    
    // Step 2: Wait for "OK Ready"
    let mut response = String::new();
    let mut buf = [0u8; 1];
    port.set_timeout(Duration::from_secs(5))?;
    
    while !response.ends_with('\n') {
        port.read_exact(&mut buf)?;
        response.push(buf[0] as char);
    }
    
    if !response.trim().starts_with("OK Ready") {
        return Err(format!("Unexpected response: {}", response).into());
    }
    
    // Step 3: Send firmware binary
    const CHUNK_SIZE: usize = 512;
    for (i, chunk) in firmware.chunks(CHUNK_SIZE).enumerate() {
        port.write_all(chunk)?;
        port.flush()?;
        
        // Progress callback
        let progress = ((i + 1) * CHUNK_SIZE * 100) / firmware.len();
        println!("Progress: {}%", progress.min(100));
        
        // Small delay to avoid overwhelming device
        std::thread::sleep(Duration::from_millis(10));
    }
    
    // Step 4: Wait for completion
    response.clear();
    port.set_timeout(Duration::from_secs(30))?;
    
    while !response.ends_with('\n') {
        port.read_exact(&mut buf)?;
        response.push(buf[0] as char);
    }
    
    if !response.trim().starts_with("OK Complete") {
        return Err(format!("Upload failed: {}", response).into());
    }
    
    println!("Firmware uploaded successfully, device rebooting...");
    Ok(())
}
```

---

## Usage Example

```rust
use serialport::SerialPortBuilder;
use std::fs;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Open serial port
    let mut port = serialport::new("/dev/ttyACM0", 115200)
        .timeout(Duration::from_secs(5))
        .open()?;
    
    // Read firmware file
    let firmware = fs::read("firmware.bin")?;
    
    println!("Uploading {} bytes...", firmware.len());
    
    // Upload
    upload_firmware(&mut port, &firmware)?;
    
    Ok(())
}
```

---

## Error Handling

```rust
#[derive(Debug, thiserror::Error)]
pub enum FirmwareUploadError {
    #[error("Serial port error: {0}")]
    SerialError(#[from] serialport::Error),
    
    #[error("IO error: {0}")]
    IoError(#[from] std::io::Error),
    
    #[error("Device not ready: {0}")]
    NotReady(String),
    
    #[error("Upload failed: {0}")]
    UploadFailed(String),
    
    #[error("Timeout waiting for response")]
    Timeout,
}
```

---

## Progress Tracking

```rust
pub fn upload_firmware_with_progress<F>(
    port: &mut Box<dyn SerialPort>,
    firmware: &[u8],
    mut progress_callback: F,
) -> Result<(), FirmwareUploadError>
where
    F: FnMut(usize, usize), // (bytes_sent, total_bytes)
{
    // Send command
    let cmd = format!("FIRMWARE_UPGRADE {}\n", firmware.len());
    port.write_all(cmd.as_bytes())?;
    port.flush()?;
    
    // Wait for ready
    wait_for_response(port, "OK Ready")?;
    
    // Send firmware with progress
    const CHUNK_SIZE: usize = 512;
    let mut sent = 0;
    
    for chunk in firmware.chunks(CHUNK_SIZE) {
        port.write_all(chunk)?;
        port.flush()?;
        
        sent += chunk.len();
        progress_callback(sent, firmware.len());
        
        std::thread::sleep(Duration::from_millis(10));
    }
    
    // Wait for completion
    wait_for_response(port, "OK Complete")?;
    
    Ok(())
}

fn wait_for_response(
    port: &mut Box<dyn SerialPort>,
    expected: &str,
) -> Result<(), FirmwareUploadError> {
    let mut response = String::new();
    let mut buf = [0u8; 1];
    
    while !response.ends_with('\n') {
        match port.read_exact(&mut buf) {
            Ok(_) => response.push(buf[0] as char),
            Err(e) => return Err(FirmwareUploadError::Timeout),
        }
    }
    
    if !response.trim().starts_with(expected) {
        return Err(FirmwareUploadError::UploadFailed(response));
    }
    
    Ok(())
}
```

---

## UI Integration

```rust
// In your UI code
fn upload_firmware_ui(port_name: &str, firmware_path: &str) {
    let firmware = match fs::read(firmware_path) {
        Ok(data) => data,
        Err(e) => {
            show_error(&format!("Failed to read firmware: {}", e));
            return;
        }
    };
    
    let mut port = match serialport::new(port_name, 115200)
        .timeout(Duration::from_secs(5))
        .open() {
        Ok(p) => p,
        Err(e) => {
            show_error(&format!("Failed to open port: {}", e));
            return;
        }
    };
    
    // Show progress dialog
    show_progress_dialog("Uploading firmware...");
    
    let result = upload_firmware_with_progress(
        &mut port,
        &firmware,
        |sent, total| {
            let percentage = (sent * 100) / total;
            update_progress(percentage, &format!("{}/{} bytes", sent, total));
        },
    );
    
    match result {
        Ok(_) => show_success("Firmware uploaded! Device rebooting..."),
        Err(e) => show_error(&format!("Upload failed: {}", e)),
    }
}
```

---

## Testing

### Manual Test
```bash
# 1. Connect device via USB
# 2. Find port
ls /dev/ttyACM*

# 3. Test command manually
echo "FIRMWARE_UPGRADE 1234567" > /dev/ttyACM0
cat /dev/ttyACM0
# Should see: OK Ready

# 4. Send binary (use your tool)
```

### Automated Test
```rust
#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_upload_small_firmware() {
        let firmware = vec![0u8; 1024]; // 1KB test firmware
        let mut port = open_test_port();
        
        let result = upload_firmware(&mut port, &firmware);
        assert!(result.is_ok());
    }
}
```

---

## Key Differences from XMODEM

| Feature | XMODEM | Simple Binary |
|---------|--------|---------------|
| Protocol | Complex state machine | None |
| Overhead | ACK/NAK per block | None |
| Speed | Slow (waits for ACK) | Fast (continuous) |
| Code | 300+ lines | 60 lines |
| Reliability | Good | Excellent (USB) |

---

## Troubleshooting

### Device Not Responding
```rust
// Add timeout and retry
port.set_timeout(Duration::from_secs(10))?;
```

### Upload Stalls
```rust
// Reduce chunk size
const CHUNK_SIZE: usize = 256; // Instead of 512
```

### Wrong Response
```rust
// Log all responses for debugging
println!("Device response: {:?}", response);
```

---

## Complete Example

```rust
use serialport::{SerialPort, SerialPortBuilder};
use std::fs;
use std::io::{Read, Write};
use std::time::Duration;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Parse arguments
    let args: Vec<String> = std::env::args().collect();
    if args.len() != 3 {
        eprintln!("Usage: {} <port> <firmware.bin>", args[0]);
        std::process::exit(1);
    }
    
    let port_name = &args[1];
    let firmware_path = &args[2];
    
    // Read firmware
    println!("Reading firmware from {}...", firmware_path);
    let firmware = fs::read(firmware_path)?;
    println!("Firmware size: {} bytes", firmware.len());
    
    // Open port
    println!("Opening port {}...", port_name);
    let mut port = serialport::new(port_name, 115200)
        .timeout(Duration::from_secs(5))
        .open()?;
    
    // Upload
    println!("Starting upload...");
    upload_firmware_with_progress(
        &mut port,
        &firmware,
        |sent, total| {
            let pct = (sent * 100) / total;
            print!("\rProgress: {}% ({}/{} bytes)", pct, sent, total);
            std::io::stdout().flush().unwrap();
        },
    )?;
    
    println!("\n✅ Upload complete!");
    Ok(())
}
```

---

## Summary

**What to implement:**
1. Send `FIRMWARE_UPGRADE <size>\n`
2. Wait for `OK Ready\n`
3. Send raw binary data
4. Wait for `OK Complete\n`

**That's it. No protocol, no complexity, just works.**

---

## Files to Modify in LoRaCueManager

```
src/
├── firmware/
│   ├── mod.rs           # Add firmware module
│   └── upload.rs        # Implement upload_firmware()
└── ui/
    └── firmware_view.rs # Add UI for firmware upload
```

---

## Next Steps

1. Add `serialport` dependency to `Cargo.toml`
2. Implement `upload_firmware()` function
3. Add UI button "Upload Firmware"
4. Test with real device
5. Add progress bar
6. Handle errors gracefully
