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
    frequency: 915000000,
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

  const getFrequencyLabel = (freq: number) => {
    if (freq === 868000000) return '868 MHz (EU)'
    if (freq === 915000000) return '915 MHz (US)'
    if (freq === 433000000) return '433 MHz (China/World)'
    return `${freq / 1000000} MHz`
  }

  const bandwidthOptions = [
    { value: 125000, label: '125 kHz' },
    { value: 250000, label: '250 kHz' },
    { value: 500000, label: '500 kHz' }
  ]

  const presets = [
    { name: 'Conference (50m)', sf: 7, bw: 500000, power: 14, desc: 'Fast, low latency' },
    { name: 'Auditorium (200m)', sf: 8, bw: 250000, power: 17, desc: 'Balanced' },
    { name: 'Stadium (500m)', sf: 10, bw: 125000, power: 20, desc: 'Max range' }
  ]

  const applyPreset = (preset: typeof presets[0]) => {
    setSettings({
      ...settings,
      spreadingFactor: preset.sf,
      bandwidth: preset.bw,
      txPower: preset.power
    })
  }

  const calculatePerformance = () => {
    const sf = settings.spreadingFactor
    const bw = settings.bandwidth // in Hz
    const cr = settings.codingRate
    const pl = 10 // payload length in bytes
    
    // Symbol duration in seconds
    const Ts = Math.pow(2, sf) / bw
    
    // Preamble time (8 symbols + 4.25 symbols)
    const Tpreamble = (8 + 4.25) * Ts
    
    // Payload symbol count
    const payloadSymbNb = 8 + Math.max(
      Math.ceil((8 * pl - 4 * sf + 28 + 16) / (4 * sf)) * cr,
      0
    )
    
    // Payload time
    const Tpayload = payloadSymbNb * Ts
    
    // Total time on air in milliseconds
    const timeOnAir = (Tpreamble + Tpayload) * 1000
    
    // Range estimation based on SF and TX power
    let range = 50
    if (sf >= 10 && settings.txPower >= 17) range = 500
    else if (sf >= 9 && settings.txPower >= 17) range = 300
    else if (sf >= 8 && settings.txPower >= 14) range = 200
    else if (sf >= 7) range = 50
    
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
                  Frequency (Hardware)
                </label>
                <div className="input bg-gray-100 dark:bg-gray-800 cursor-not-allowed">
                  {getFrequencyLabel(settings.frequency)}
                </div>
                <p className="text-sm text-gray-500 mt-1">
                  Determined by LoRa module hardware
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
