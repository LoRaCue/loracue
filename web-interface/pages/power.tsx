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

  return (
    <Layout>
      <div className="space-y-8">
        <div>
          <h1 className="text-3xl font-bold">Power Management</h1>
          <p className="text-gray-500 dark:text-gray-400 mt-2">
            Configure sleep modes to optimize battery life
          </p>
        </div>

        <div className="card space-y-6">
          {/* Display Sleep */}
          <div className="space-y-4 pb-6 border-b border-gray-200 dark:border-gray-700">
            <div className="flex items-center space-x-3">
              <Moon className="w-5 h-5 text-blue-500" />
              <h2 className="text-xl font-semibold">Display Sleep</h2>
            </div>
            <p className="text-sm text-gray-500 dark:text-gray-400">
              Turn off OLED display after inactivity to save power
            </p>
            
            <div>
              <label className="flex items-center space-x-2">
                <input
                  type="checkbox"
                  checked={settings.display_sleep_enabled}
                  onChange={(e) => setSettings({ ...settings, display_sleep_enabled: e.target.checked })}
                  className="rounded"
                />
                <span className="text-sm font-medium">Enable Display Sleep</span>
              </label>
            </div>

            {settings.display_sleep_enabled && (
              <div>
                <label className="block text-sm font-medium mb-2">
                  Timeout (seconds)
                </label>
                <input
                  type="number"
                  value={settings.display_sleep_timeout_ms / 1000}
                  onChange={(e) => setSettings({ ...settings, display_sleep_timeout_ms: parseInt(e.target.value) * 1000 })}
                  className="input"
                  min="5"
                  max="300"
                />
                <p className="text-xs text-gray-500 dark:text-gray-400 mt-1">
                  Display turns off after {settings.display_sleep_timeout_ms / 1000}s of inactivity
                </p>
              </div>
            )}
          </div>

          {/* Light Sleep */}
          <div className="space-y-4 pb-6 border-b border-gray-200 dark:border-gray-700">
            <div className="flex items-center space-x-3">
              <Zap className="w-5 h-5 text-yellow-500" />
              <h2 className="text-xl font-semibold">Light Sleep</h2>
            </div>
            <p className="text-sm text-gray-500 dark:text-gray-400">
              Reduce CPU frequency and disable peripherals. Wake instantly on button press.
            </p>
            
            <div>
              <label className="flex items-center space-x-2">
                <input
                  type="checkbox"
                  checked={settings.light_sleep_enabled}
                  onChange={(e) => setSettings({ ...settings, light_sleep_enabled: e.target.checked })}
                  className="rounded"
                />
                <span className="text-sm font-medium">Enable Light Sleep</span>
              </label>
            </div>

            {settings.light_sleep_enabled && (
              <div>
                <label className="block text-sm font-medium mb-2">
                  Timeout (seconds)
                </label>
                <input
                  type="number"
                  value={settings.light_sleep_timeout_ms / 1000}
                  onChange={(e) => setSettings({ ...settings, light_sleep_timeout_ms: parseInt(e.target.value) * 1000 })}
                  className="input"
                  min="10"
                  max="600"
                />
                <p className="text-xs text-gray-500 dark:text-gray-400 mt-1">
                  Enter light sleep after {settings.light_sleep_timeout_ms / 1000}s of inactivity
                </p>
              </div>
            )}
          </div>

          {/* Deep Sleep */}
          <div className="space-y-4">
            <div className="flex items-center space-x-3">
              <Power className="w-5 h-5 text-red-500" />
              <h2 className="text-xl font-semibold">Deep Sleep</h2>
            </div>
            <p className="text-sm text-gray-500 dark:text-gray-400">
              Maximum power savings. Device fully powers down. Press button to wake.
            </p>
            
            <div>
              <label className="flex items-center space-x-2">
                <input
                  type="checkbox"
                  checked={settings.deep_sleep_enabled}
                  onChange={(e) => setSettings({ ...settings, deep_sleep_enabled: e.target.checked })}
                  className="rounded"
                />
                <span className="text-sm font-medium">Enable Deep Sleep</span>
              </label>
            </div>

            {settings.deep_sleep_enabled && (
              <div>
                <label className="block text-sm font-medium mb-2">
                  Timeout (seconds)
                </label>
                <input
                  type="number"
                  value={settings.deep_sleep_timeout_ms / 1000}
                  onChange={(e) => setSettings({ ...settings, deep_sleep_timeout_ms: parseInt(e.target.value) * 1000 })}
                  className="input"
                  min="60"
                  max="3600"
                />
                <p className="text-xs text-gray-500 dark:text-gray-400 mt-1">
                  Enter deep sleep after {settings.deep_sleep_timeout_ms / 1000}s of inactivity
                </p>
              </div>
            )}
          </div>

          <div className="flex items-center justify-between pt-4 border-t border-gray-200 dark:border-gray-700">
            <div className="flex items-center space-x-2">
              {status === 'success' && (
                <>
                  <CheckCircle className="w-5 h-5 text-green-500" />
                  <span className="text-sm text-green-600 dark:text-green-400">Settings saved successfully</span>
                </>
              )}
              {status === 'error' && (
                <>
                  <AlertCircle className="w-5 h-5 text-red-500" />
                  <span className="text-sm text-red-600 dark:text-red-400">Failed to save settings</span>
                </>
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
      </div>
    </Layout>
  )
}
