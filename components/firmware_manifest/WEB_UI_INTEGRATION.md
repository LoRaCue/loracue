# Web UI Integration for Firmware Compatibility

## Required Changes to web-interface/pages/firmware.tsx

### 1. Parse Manifest from Binary File

Add function to extract manifest from uploaded .bin file:

```typescript
async function extractManifest(file: File): Promise<FirmwareManifest | null> {
  // Read first 4KB of file
  const header = await file.slice(0, 4096).arrayBuffer()
  const view = new DataView(header)
  
  // Search for magic number 0x4C524355 ("LRCU")
  for (let i = 0; i < header.byteLength - 60; i++) {
    if (view.getUint32(i, true) === 0x4C524355) {
      // Found manifest, parse it
      const boardId = new TextDecoder().decode(
        new Uint8Array(header, i + 8, 16)
      ).replace(/\0/g, '')
      
      const version = new TextDecoder().decode(
        new Uint8Array(header, i + 24, 32)
      ).replace(/\0/g, '')
      
      return { boardId, version }
    }
  }
  return null
}
```

### 2. Show Compatibility Warning

Add state and UI for compatibility check:

```typescript
const [manifest, setManifest] = useState<FirmwareManifest | null>(null)
const [forceMode, setForceMode] = useState(false)
const [compatWarning, setCompatWarning] = useState<string | null>(null)

// After file selection:
const manifest = await extractManifest(selectedFile)
if (manifest) {
  // Fetch current board ID from device
  const deviceInfo = await fetch('/api/system/info').then(r => r.json())
  
  if (manifest.boardId !== deviceInfo.board_id) {
    setCompatWarning(
      `Board mismatch: Device is ${deviceInfo.board_id}, firmware is for ${manifest.boardId}`
    )
  }
}
```

### 3. Add Force Mode Checkbox

```tsx
{compatWarning && (
  <div className="bg-yellow-50 border border-yellow-200 rounded p-4">
    <AlertCircle className="inline mr-2" />
    <span>{compatWarning}</span>
    <label className="block mt-2">
      <input 
        type="checkbox" 
        checked={forceMode}
        onChange={(e) => setForceMode(e.target.checked)}
      />
      Force update anyway (advanced)
    </label>
  </div>
)}
```

### 4. Send Force Parameter

```typescript
const startRes = await fetch('/api/firmware/start', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ 
    size: file.size,
    force: forceMode  // Add this
  })
})
```

## Backend API Changes

The backend already supports the `force` parameter in FW_UPDATE_START command.
No changes needed to config_wifi_server.c - it passes through to commands.c.
