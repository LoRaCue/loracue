import { useState, useEffect } from 'react'
import Layout from '../components/Layout'
import { Save, Loader2, CheckCircle, AlertCircle, Power, Moon, Zap } from 'lucide-react'

interface PowerSettings {
  display_sleep_enabled: boolean
  display_sleep_timeout_ms: number
  light_sleep_enabled: boolean
  light_sleep_timeout_ms: number
  deep_sleep_enabled: boolean
  deep_sleep_timeout_ms: number
}

export default function PowerPage() {
  const [settings, setSettings] = useState<PowerSettings>({
    display_sleep_enabled: true,
    display_sleep_timeout_ms: 10000,
    light_sleep_enabled: true,
    light_sleep_timeout_ms: 30000,
    deep_sleep_enabled: true,
    deep_sleep_timeout_ms: 300000
  })
  const [loading, setLoading] = useState(false)
  const [status, setStatus] = useState<'idle' | 'success' | 'error'>('idle')

  useEffect(() => {
    fetch('/api/power-management')
      .then(res => res.json())
      .then(data => setSettings(data))
      .catch(() => {})
  }, [])

  const handleSave = async () => {
    setLoading(true)
    setStatus('idle')
    
    try {
      const response = await fetch('/api/power-management', {
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

  const formatTime = (ms: number) => {
    if (ms < 60000) return `${(ms / 1000).toFixed(0)}s`
    if (ms < 3600000) return `${(ms / 60000).toFixed(0)}min`
    return `${(ms / 3600000).toFixed(1)}h`
  }

  return (
    <Layout>
      <div className="space-y-8">
        <div>
          <h1 className="text-3xl font-bold">Power Management</h1>
          <p className="text-gray-500 dark:text-gray-400 mt-2">
            Configure sleep modes to optimize battery life
          </p>
        </div>

        <div className="card p-8 space-y-6">
          {/* Display Sleep */}
          <div className="space-y-4 pb-6 border-b border-gray-200 dark:border-gray-700">
            <div className="flex items-center justify-between">
              <div className="flex items-center space-x-3">
                <Moon className="w-5 h-5 text-blue-500" />
                <h2 className="text-xl font-semibold">Display Sleep</h2>
              </div>
              <label className="relative inline-flex items-center cursor-pointer">
                <input
                  type="checkbox"
                  checked={settings.display_sleep_enabled}
                  onChange={(e) => setSettings({...settings, display_sleep_enabled: e.target.checked})}
                  className="sr-only peer"
                />
                <div className="w-11 h-6 bg-gray-200 peer-focus:outline-none peer-focus:ring-4 peer-focus:ring-blue-300 dark:peer-focus:ring-blue-800 rounded-full peer dark:bg-gray-700 peer-checked:after:translate-x-full rtl:peer-checked:after:-translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:start-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-5 after:w-5 after:transition-all dark:border-gray-600 peer-checked:bg-blue-600"></div>
              </label>
            </div>
            <p className="text-sm text-gray-500 dark:text-gray-400">
              Turn off OLED display after inactivity to save power
            </p>
            
            {settings.display_sleep_enabled && (
              <div className="space-y-2">
                <label className="text-sm font-medium">
                  Timeout: {formatTime(settings.display_sleep_timeout_ms)}
                </label>
                <input
                  type="range"
                  min="5000"
                  max="21600000"
                  step="5000"
                  value={settings.display_sleep_timeout_ms}
                  onChange={(e) => setSettings({...settings, display_sleep_timeout_ms: parseInt(e.target.value)})}
                  className="w-full h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer dark:bg-gray-700"
                />
              </div>
            )}
          </div>

          {/* Light Sleep */}
          <div className="space-y-4 pb-6 border-b border-gray-200 dark:border-gray-700">
            <div className="flex items-center justify-between">
              <div className="flex items-center space-x-3">
                <Zap className="w-5 h-5 text-yellow-500" />
                <h2 className="text-xl font-semibold">Light Sleep</h2>
              </div>
              <label className="relative inline-flex items-center cursor-pointer">
                <input
                  type="checkbox"
                  checked={settings.light_sleep_enabled}
                  onChange={(e) => setSettings({...settings, light_sleep_enabled: e.target.checked})}
                  className="sr-only peer"
                />
                <div className="w-11 h-6 bg-gray-200 peer-focus:outline-none peer-focus:ring-4 peer-focus:ring-blue-300 dark:peer-focus:ring-blue-800 rounded-full peer dark:bg-gray-700 peer-checked:after:translate-x-full rtl:peer-checked:after:-translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:start-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-5 after:w-5 after:transition-all dark:border-gray-600 peer-checked:bg-blue-600"></div>
              </label>
            </div>
            <p className="text-sm text-gray-500 dark:text-gray-400">
              Enter low-power mode while maintaining quick wake-up
            </p>
            
            {settings.light_sleep_enabled && (
              <div className="space-y-2">
                <label className="text-sm font-medium">
                  Timeout: {formatTime(settings.light_sleep_timeout_ms)}
                </label>
                <input
                  type="range"
                  min="10000"
                  max="21600000"
                  step="10000"
                  value={settings.light_sleep_timeout_ms}
                  onChange={(e) => setSettings({...settings, light_sleep_timeout_ms: parseInt(e.target.value)})}
                  className="w-full h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer dark:bg-gray-700"
                />
              </div>
            )}
          </div>

          {/* Deep Sleep */}
          <div className="space-y-4">
            <div className="flex items-center justify-between">
              <div className="flex items-center space-x-3">
                <Power className="w-5 h-5 text-red-500" />
                <h2 className="text-xl font-semibold">Deep Sleep</h2>
              </div>
              <label className="relative inline-flex items-center cursor-pointer">
                <input
                  type="checkbox"
                  checked={settings.deep_sleep_enabled}
                  onChange={(e) => setSettings({...settings, deep_sleep_enabled: e.target.checked})}
                  className="sr-only peer"
                />
                <div className="w-11 h-6 bg-gray-200 peer-focus:outline-none peer-focus:ring-4 peer-focus:ring-blue-300 dark:peer-focus:ring-blue-800 rounded-full peer dark:bg-gray-700 peer-checked:after:translate-x-full rtl:peer-checked:after:-translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:start-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-5 after:w-5 after:transition-all dark:border-gray-600 peer-checked:bg-blue-600"></div>
              </label>
            </div>
            <p className="text-sm text-gray-500 dark:text-gray-400">
              Maximum power savings - requires button press to wake
            </p>
            
            {settings.deep_sleep_enabled && (
              <div className="space-y-2">
                <label className="text-sm font-medium">
                  Timeout: {formatTime(settings.deep_sleep_timeout_ms)}
                </label>
                <input
                  type="range"
                  min="60000"
                  max="21600000"
                  step="60000"
                  value={settings.deep_sleep_timeout_ms}
                  onChange={(e) => setSettings({...settings, deep_sleep_timeout_ms: parseInt(e.target.value)})}
                  className="w-full h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer dark:bg-gray-700"
                />
              </div>
            )}
          </div>
        </div>

        {/* Status and Save Button */}
        <div className="flex items-center justify-between">
          <div>
            {status === 'success' && (
              <div className="flex items-center space-x-2 text-green-600 dark:text-green-400">
                <CheckCircle className="w-5 h-5" />
                <span>Settings saved successfully!</span>
              </div>
            )}
            
            {status === 'error' && (
              <div className="flex items-center space-x-2 text-red-600 dark:text-red-400">
                <AlertCircle className="w-5 h-5" />
                <span>Failed to save settings. Please try again.</span>
              </div>
            )}
          </div>

          <button
            onClick={handleSave}
            disabled={loading}
            className="btn-primary flex items-center space-x-2"
          >
            {loading ? (
              <>
                <Loader2 className="w-4 h-4 animate-spin" />
                <span>Saving...</span>
              </>
            ) : (
              <>
                <Save className="w-4 h-4" />
                <span>Save Settings</span>
              </>
            )}
          </button>
        </div>
      </div>
    </Layout>
  )
}
