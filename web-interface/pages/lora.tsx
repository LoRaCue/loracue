import { useState, useEffect } from 'react'
import Layout from '../components/Layout'
import { Save, Loader2, CheckCircle, AlertCircle, Radio } from 'lucide-react'

interface LoRaSettings {
  frequency: number
  spreadingFactor: number
  bandwidth: number
  codingRate: number
  txPower: number
  syncWord: number
}

export default function LoRaPage() {
  const [settings, setSettings] = useState<LoRaSettings>({
    frequency: 868000000,
    spreadingFactor: 7,
    bandwidth: 500000,
    codingRate: 5,
    txPower: 14,
    syncWord: 0x12
  })
  const [loading, setLoading] = useState(false)
  const [status, setStatus] = useState<'idle' | 'success' | 'error'>('idle')

  useEffect(() => {
    fetch('/api/lora/settings')
      .then(res => res.json())
      .then(data => setSettings(data))
      .catch(() => {})
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
    { name: 'Conference (100m)', sf: 7, bw: 500000, cr: 5, desc: 'Fast, low latency' },
    { name: 'Auditorium (250m)', sf: 9, bw: 125000, cr: 7, desc: 'Balanced range' },
    { name: 'Stadium (500m)', sf: 10, bw: 125000, cr: 8, desc: 'Maximum range' }
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
      txPower: getTxPowerForFrequency(settings.frequency)
    })
  }

  const calculatePerformance = () => {
    const sf = settings.spreadingFactor
    const bw = settings.bandwidth // in Hz
    const cr = settings.codingRate - 4 // Convert 5-8 to 1-4
    const pl = 10 // payload length in bytes
    const crc = 1 // CRC enabled
    const h = 0 // Explicit header
    const de = (sf >= 11 && bw === 125000) ? 1 : 0 // Low data rate optimization
    
    // Symbol duration in seconds
    const Ts = Math.pow(2, sf) / bw
    
    // Preamble time (8 symbols + 4.25 symbols)
    const Tpreamble = (8 + 4.25) * Ts
    
    // Payload symbol count (Semtech formula)
    const payloadSymbNb = 8 + Math.max(
      Math.ceil((8 * pl - 4 * sf + 28 + 16 * crc - 20 * h) / (4 * (sf - 2 * de))) * (cr + 4),
      0
    )
    
    // Total time on air in milliseconds
    const timeOnAir = (Tpreamble + payloadSymbNb * Ts) * 1000
    
    // Range estimation using link budget
    // SX1262 sensitivity: -148 dBm @ SF12/BW125, improves ~2.5dB per SF step down
    const sensitivity = -148 + (12 - sf) * 2.5 + (bw > 125000 ? Math.log2(bw / 125000) * 3 : 0)
    const txPower = settings.txPower
    const antennaGain = 2 // Small external antenna (dBi)
    const linkBudget = txPower + antennaGain - sensitivity + antennaGain
    
    // Free-space path loss: FSPL = 20*log10(d) + 20*log10(f) + 20*log10(4π/c)
    // Solving for d: d = 10^((FSPL - 20*log10(f) - 20*log10(4π/c)) / 20)
    const freq = settings.frequency
    const c = 299792458 // speed of light
    const fsplConstant = 20 * Math.log10(freq) + 20 * Math.log10(4 * Math.PI / c)
    const range = Math.pow(10, (linkBudget - fsplConstant) / 20)
    
    return {
      latency: Math.round(timeOnAir),
      range: range
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
            {presets.map((preset, idx) => (
              <button
                key={idx}
                onClick={() => applyPreset(preset)}
                className="p-4 border-2 border-gray-200 dark:border-gray-700 rounded-lg hover:border-blue-500 dark:hover:border-blue-400 transition-colors text-left"
              >
                <div className="font-semibold mb-1">{preset.name}</div>
                <div className="text-sm text-gray-600 dark:text-gray-400 mb-2">{preset.desc}</div>
                <div className="text-xs text-gray-500">
                  SF{preset.sf}, {preset.bw/1000}kHz, {preset.power}dBm
                </div>
              </button>
            ))}
          </div>
        </div>

        <div className="card p-8">
          <h2 className="text-xl font-semibold mb-6">Advanced Settings</h2>
          <div className="space-y-6">
            <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
              <div>
                <label className="block text-sm font-medium mb-2">
                  Frequency
                </label>
                <select
                  value={settings.frequency}
                  onChange={(e) => setSettings({ ...settings, frequency: Number(e.target.value) })}
                  className="input"
                >
                  {frequencyOptions.map(option => (
                    <option key={option.value} value={option.value}>
                      {option.label}
                    </option>
                  ))}
                </select>
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
                <label className="block text-sm font-medium mb-2">
                  TX Power (dBm)
                </label>
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

              <div>
                <label className="block text-sm font-medium mb-2">
                  Sync Word (Hex)
                </label>
                <input
                  type="text"
                  value={`0x${settings.syncWord.toString(16).toUpperCase().padStart(2, '0')}`}
                  onChange={(e) => {
                    const hex = e.target.value.replace('0x', '').replace(/[^0-9A-Fa-f]/g, '')
                    if (hex.length <= 2) {
                      setSettings({ ...settings, syncWord: parseInt(hex || '0', 16) })
                    }
                  }}
                  className="input font-mono"
                  placeholder="0x12"
                />
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
    </Layout>
  )
}
