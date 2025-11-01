import { useState, useEffect } from 'react'
import Layout from '../components/Layout'
import { Save, Loader2, CheckCircle, AlertCircle, Radio, Key, Copy, RefreshCw } from 'lucide-react'

interface LoRaSettings {
  frequency: number
  spreadingFactor: number
  bandwidth: number
  codingRate: number
  txPower: number
  band_id: string
  aes_key: string
}

interface Band {
  id: string
  name: string
  center_khz: number
  min_khz: number
  max_khz: number
  max_power_dbm: number
}

export default function LoRaPage() {
  const [settings, setSettings] = useState<LoRaSettings>({
    frequency: 868000000,
    spreadingFactor: 7,
    bandwidth: 500000,
    codingRate: 5,
    txPower: 14,
    band_id: 'HW_868',
    aes_key: ''
  })
  const [bands, setBands] = useState<Band[]>([])
  const [loading, setLoading] = useState(false)
  const [status, setStatus] = useState<'idle' | 'success' | 'error'>('idle')
  const [showBandWarning, setShowBandWarning] = useState(false)
  const [pendingBandId, setPendingBandId] = useState<string | null>(null)

  const handleBandChange = (bandId: string) => {
    setPendingBandId(bandId)
    setShowBandWarning(true)
  }

  const confirmBandChange = () => {
    if (pendingBandId) {
      const band = bands.find(b => b.id === pendingBandId)
      if (band) {
        setSettings({ 
          ...settings, 
          band_id: band.id,
          frequency: band.center_khz * 1000
        })
      }
    }
    setShowBandWarning(false)
    setPendingBandId(null)
  }

  useEffect(() => {
    // Fetch current settings
    fetch('/api/lora/settings')
      .then(res => res.ok ? res.json() : Promise.reject())
      .then(data => setSettings(data))
      .catch(() => {})
    
    // Fetch available bands
    fetch('/api/lora/bands')
      .then(res => res.ok ? res.json() : Promise.reject())
      .then(data => {

        setBands(data)
      })
      .catch((err) => {
        // Fallback to default bands if API not available
        setBands([
          { id: 'HW_433', name: '433 MHz Band', center_khz: 433000, min_khz: 430000, max_khz: 440000, max_power_dbm: 10 },
          { id: 'HW_868', name: '868 MHz Band', center_khz: 868000, min_khz: 863000, max_khz: 870000, max_power_dbm: 14 },
          { id: 'HW_915', name: '915 MHz Band', center_khz: 915000, min_khz: 902000, max_khz: 928000, max_power_dbm: 20 }
        ])
      })
  }, [])

  const handleSave = async () => {
    setLoading(true)
    setStatus('idle')
    
    try {
      const response = await fetch('/api/lora/settings', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(settings)
      })
      
      if (response.ok) {
        setStatus('success')
        setTimeout(() => setStatus('idle'), 3000)
      } else {
        setStatus('error')
      }
    } catch (error) {
      setStatus('error')
    } finally {
      setLoading(false)
    }
  }

  const frequencyOptions = [
    { value: 868000000, label: '868 MHz (EU)' },
    { value: 915000000, label: '915 MHz (US)' },
    { value: 433000000, label: '433 MHz' }
  ]

  const bandwidthOptions = [
    { value: 125000, label: '125 kHz' },
    { value: 250000, label: '250 kHz' },
    { value: 500000, label: '500 kHz' }
  ]

  const presets = [
    { name: 'Conference (100m)', sf: 7, bw: 500000, cr: 5, power: 14, desc: 'Fast, low latency' },
    { name: 'Auditorium (250m)', sf: 9, bw: 125000, cr: 7, power: 14, desc: 'Balanced range' },
    { name: 'Stadium (500m)', sf: 10, bw: 125000, cr: 8, power: 17, desc: 'Maximum range' }
  ]

  // Get TX power based on frequency
  const getTxPowerForFrequency = (freq: number): number => {
    if (freq >= 430000000 && freq <= 440000000) return 10  // 433MHz: 10dBm
    if (freq >= 863000000 && freq <= 870000000) return 14  // 868MHz: 14dBm
    if (freq >= 902000000 && freq <= 928000000) return 17  // 915MHz: 17dBm
    return 14 // Default
  }

  // Update power when frequency changes
  useEffect(() => {
    const newPower = getTxPowerForFrequency(settings.frequency)
    if (newPower !== settings.txPower) {
      setSettings(prev => ({ ...prev, txPower: newPower }))
    }
  }, [settings.frequency])

  const applyPreset = (preset: typeof presets[0]) => {
    setSettings({
      ...settings,
      spreadingFactor: preset.sf,
      bandwidth: preset.bw,
      codingRate: preset.cr,
      txPower: preset.power
    })
  }

  // Check if current settings match a preset
  const getActivePreset = () => {
    return presets.find(p => 
      p.sf === settings.spreadingFactor &&
      p.bw === settings.bandwidth &&
      p.cr === settings.codingRate &&
      p.power === settings.txPower
    )
  }

  const activePreset = getActivePreset()

  const calculatePerformance = () => {
    const sf = settings.spreadingFactor
    const bw = settings.bandwidth // in Hz
    const cr = settings.codingRate // 5-8 (represents 4/5 to 4/8)
    const pl = 16 // payload length in bytes (typical for button press)
    const preambleLength = 8 // symbols
    const crc = 1 // CRC enabled
    const implicitHeader = 0 // Explicit header
    
    // Symbol time in seconds
    const Ts = Math.pow(2, sf) / bw
    
    // Preamble time
    const Tpreamble = (preambleLength + 4.25) * Ts
    
    // Payload symbols
    const payloadSymbNb = 8 + Math.max(
      Math.ceil(
        (8 * pl - 4 * sf + 28 + 16 * crc - 20 * implicitHeader) / 
        (4 * (sf - 0))
      ) * (cr + 4),
      0
    )
    
    // Total time on air in milliseconds
    const timeOnAir = (Tpreamble + payloadSymbNb * Ts) * 1000
    
    // Range estimation for indoor use (conservative)
    // SX1262 sensitivity: -148 dBm @ SF12/BW125, improves ~2.5dB per SF step down
    const sensitivity = -148 + (12 - sf) * 2.5 + (bw > 125000 ? Math.log2(bw / 125000) * 3 : 0)
    const txPower = settings.txPower
    const linkBudget = txPower - sensitivity
    
    // Indoor path loss model with heavy attenuation
    // Path loss exponent: 3.5 (concrete walls, multiple floors)
    // Fade margin: 20 dB (conservative for reliability)
    // Reference loss: 50 dB at 1m (realistic for indoor 868 MHz)
    const fadeMargin = 20 // dB - conservative
    const pathLossExponent = 3.5 // Heavy indoor attenuation
    const referenceDistance = 1 // meters
    const referenceLoss = 50 // dB at 1m
    
    // Solve for distance: d = d0 * 10^((linkBudget - fadeMargin - PL0) / (10*n))
    const range = referenceDistance * Math.pow(10, (linkBudget - fadeMargin - referenceLoss) / (10 * pathLossExponent))
    
    return {
      latency: Math.round(timeOnAir),
      range: Math.round(range) // Full meters only
    }
  }

  const perf = calculatePerformance()

  return (
    <Layout>
      <div className="space-y-8">
        <div>
          <h1 className="text-3xl font-bold mb-2">LoRa Configuration</h1>
          <p className="text-gray-600 dark:text-gray-400">
            Configure LoRa radio parameters for optimal performance
          </p>
        </div>

        {/* Presets Section */}
        <div className="card p-6">
          <h2 className="text-xl font-semibold mb-4">Quick Presets</h2>
          <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
            {presets.map((preset, idx) => {
              const isActive = activePreset?.name === preset.name
              return (
                <button
                  key={idx}
                  onClick={() => applyPreset(preset)}
                  className={`p-4 border-2 rounded-lg transition-colors text-left ${
                    isActive
                      ? 'border-blue-500 bg-blue-50 dark:bg-blue-900/20 dark:border-blue-400'
                      : 'border-gray-200 dark:border-gray-700 hover:border-blue-500 dark:hover:border-blue-400'
                  }`}
                >
                  <div className="font-semibold mb-1">{preset.name}</div>
                  <div className="text-sm text-gray-600 dark:text-gray-400 mb-2">{preset.desc}</div>
                  <div className="text-xs text-gray-500">
                    SF{preset.sf}, {preset.bw/1000}kHz, {preset.power}dBm
                  </div>
                </button>
              )
            })}
          </div>
        </div>

        <div className="card p-8">
          <h2 className="text-xl font-semibold mb-6">Advanced Settings</h2>
          <div className="space-y-6">
            <div>
              <label className="block text-sm font-medium mb-2">
                Hardware Band
              </label>
              <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
                {bands.map(band => (
                  <button
                    key={band.id}
                    onClick={() => handleBandChange(band.id)}
                    className={`p-4 border-2 rounded-lg transition-colors text-left ${
                      settings.band_id === band.id 
                        ? 'border-primary-500 bg-primary-50 dark:bg-primary-900/20' 
                        : 'border-gray-300 dark:border-gray-600 hover:border-primary-300'
                    }`}
                  >
                    <div className="font-semibold mb-1">{band.name}</div>
                    <div className="text-sm text-gray-600 dark:text-gray-400">
                      {band.min_khz / 1000} - {band.max_khz / 1000} MHz
                    </div>
                  </button>
                ))}
              </div>
            </div>

            <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
              <div>
                <label className="block text-sm font-medium mb-2">
                  Frequency (MHz)
                </label>
                <input
                  type="number"
                  value={(settings.frequency / 1000000).toFixed(1)}
                  onChange={(e) => setSettings({ ...settings, frequency: Math.round(parseFloat(e.target.value) * 1000000) })}
                  className="input"
                  step="0.1"
                  min={(bands.find(b => b.id === settings.band_id)?.min_khz || 430000) / 1000}
                  max={(bands.find(b => b.id === settings.band_id)?.max_khz || 440000) / 1000}
                />
                <p className="text-sm text-gray-500 mt-1">
                  0.1 MHz resolution (e.g., 868.1 MHz)
                </p>
              </div>

              <div>
                <label className="block text-sm font-medium mb-2">
                  Spreading Factor
                </label>
                <select
                  value={settings.spreadingFactor}
                  onChange={(e) => setSettings({ ...settings, spreadingFactor: Number(e.target.value) })}
                  className="input"
                >
                  {[7, 8, 9, 10, 11, 12].map(sf => (
                    <option key={sf} value={sf}>SF{sf}</option>
                  ))}
                </select>
                <p className="text-sm text-gray-500 mt-1">
                  Lower = faster, higher = longer range
                </p>
              </div>

              <div>
                <label className="block text-sm font-medium mb-2">
                  Bandwidth
                </label>
                <select
                  value={settings.bandwidth}
                  onChange={(e) => setSettings({ ...settings, bandwidth: Number(e.target.value) })}
                  className="input"
                >
                  {bandwidthOptions.map(option => (
                    <option key={option.value} value={option.value}>
                      {option.label}
                    </option>
                  ))}
                </select>
              </div>

              <div>
                <label className="block text-sm font-medium mb-2">
                  Coding Rate
                </label>
                <select
                  value={settings.codingRate}
                  onChange={(e) => setSettings({ ...settings, codingRate: Number(e.target.value) })}
                  className="input"
                >
                  {[5, 6, 7, 8].map(cr => (
                    <option key={cr} value={cr}>4/{cr}</option>
                  ))}
                </select>
              </div>

              <div>
                <div className="flex items-center justify-between mb-2">
                  <label className="text-sm font-medium">
                    TX Power (dBm)
                  </label>
                  {(() => {
                    const currentBand = bands.find(b => b.id === settings.band_id);
                    if (currentBand && settings.txPower > currentBand.max_power_dbm) {
                      return (
                        <div className="text-xs text-amber-600 dark:text-amber-400 flex items-center space-x-1">
                          <AlertCircle size={14} />
                          <span>Exceeds {currentBand.name} limit ({currentBand.max_power_dbm} dBm)</span>
                        </div>
                      );
                    }
                    return null;
                  })()}
                </div>
                <input
                  type="range"
                  min="2"
                  max="20"
                  value={settings.txPower}
                  onChange={(e) => setSettings({ ...settings, txPower: Number(e.target.value) })}
                  className="w-full"
                />
                <div className="flex justify-between text-sm text-gray-500 mt-1">
                  <span>2 dBm</span>
                  <span className="font-medium">{settings.txPower} dBm</span>
                  <span>20 dBm</span>
                </div>
              </div>
            </div>

            <div className="bg-blue-50 dark:bg-blue-900/20 border border-blue-200 dark:border-blue-800 rounded-lg p-4">
              <div className="flex items-start space-x-3">
                <Radio className="text-blue-600 dark:text-blue-400 mt-0.5" size={20} />
                <div>
                  <h3 className="font-medium text-blue-900 dark:text-blue-100">Performance Estimate</h3>
                  <p className="text-sm text-blue-700 dark:text-blue-300 mt-1">
                    SF{settings.spreadingFactor} + {settings.bandwidth/1000}kHz = ~{perf.latency}ms latency, 
                    ~{perf.range}m range
                  </p>
                </div>
              </div>
            </div>

            {/* AES Key Section */}
            <div className="pt-6 border-t border-gray-200 dark:border-gray-700">
              <div className="flex items-center space-x-2 mb-4">
                <Key className="text-gray-600 dark:text-gray-400" size={20} />
                <h3 className="text-lg font-semibold">AES-256 Encryption Key</h3>
              </div>
              <div className="flex space-x-2">
                <input
                  type="text"
                  value={settings.aes_key}
                  onChange={(e) => setSettings({ ...settings, aes_key: e.target.value })}
                  placeholder="64 hex characters (32 bytes)"
                  className="input flex-1 font-mono text-sm"
                  maxLength={64}
                  pattern="[0-9a-fA-F]{64}"
                />
                <button
                  onClick={() => {
                    const key = Array.from({ length: 32 }, () => 
                      Math.floor(Math.random() * 256).toString(16).padStart(2, '0')
                    ).join('')
                    setSettings({ ...settings, aes_key: key })
                  }}
                  className="btn-secondary flex items-center space-x-2"
                  title="Generate random key"
                >
                  <RefreshCw size={18} />
                  <span>Generate</span>
                </button>
                <button
                  onClick={() => {
                    if (settings.aes_key) {
                      navigator.clipboard.writeText(settings.aes_key)
                    }
                  }}
                  className="btn-secondary flex items-center space-x-2"
                  title="Copy to clipboard"
                  disabled={!settings.aes_key}
                >
                  <Copy size={18} />
                  <span>Copy</span>
                </button>
              </div>
              <p className="text-sm text-gray-500 mt-2">
                This key is used for encrypting all LoRa communications. Keep it secure and share only with paired devices.
              </p>
            </div>

            <div className="flex items-center justify-between pt-4 border-t border-gray-200 dark:border-gray-700">
              <div className="flex items-center space-x-2">
                {status === 'success' && (
                  <>
                    <CheckCircle className="text-green-500" size={20} />
                    <span className="text-green-600 dark:text-green-400">LoRa settings saved successfully</span>
                  </>
                )}
                {status === 'error' && (
                  <>
                    <AlertCircle className="text-red-500" size={20} />
                    <span className="text-red-600 dark:text-red-400">Failed to save LoRa settings</span>
                  </>
                )}
              </div>
              
              <button
                onClick={handleSave}
                disabled={loading}
                className="btn-primary disabled:opacity-50 disabled:cursor-not-allowed flex items-center space-x-2"
              >
                {loading ? <Loader2 className="animate-spin" size={18} /> : <Save size={18} />}
                <span>{loading ? 'Saving...' : 'Save Settings'}</span>
              </button>
            </div>
          </div>
        </div>
      </div>

      {/* Band Warning Dialog */}
      {showBandWarning && (
        <div className="fixed inset-0 bg-black/50 flex items-center justify-center z-50 p-4">
          <div className="bg-white dark:bg-gray-800 rounded-lg shadow-xl max-w-md w-full p-6">
            <h2 className="text-xl font-bold mb-4 flex items-center space-x-2">
              <AlertCircle className="text-amber-500" />
              <span>Hardware Band Warning</span>
            </h2>
            <p className="text-gray-700 dark:text-gray-300 mb-6">
              The selected band depends on your hardware design and cannot be changed in software alone. 
              Please select the correct band that matches your hardware.
            </p>
            <div className="flex space-x-3">
              <button
                onClick={() => {
                  setShowBandWarning(false)
                  setPendingBandId(null)
                }}
                className="flex-1 px-4 py-2 border border-gray-300 dark:border-gray-600 rounded-lg hover:bg-gray-50 dark:hover:bg-gray-700 transition-colors"
              >
                Cancel
              </button>
              <button
                onClick={confirmBandChange}
                className="flex-1 btn-primary"
              >
                I Understand
              </button>
            </div>
          </div>
        </div>
      )}
    </Layout>
  )
}
