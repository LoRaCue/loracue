import { useState, useEffect } from 'react'
import Layout from '../components/Layout'
import { Save, Loader2, CheckCircle, AlertCircle } from 'lucide-react'

interface DeviceSettings {
  name: string
  mode: string
  sleepTimeout: number
  autoSleep: boolean
  brightness: number
}

export default function DevicePage() {
  const [settings, setSettings] = useState<DeviceSettings>({
    name: '',
    mode: 'presenter',
    sleepTimeout: 300000,
    autoSleep: true,
    brightness: 128
  })
  const [loading, setLoading] = useState(false)
  const [status, setStatus] = useState<'idle' | 'success' | 'error'>('idle')

  useEffect(() => {
    fetch('/api/device/settings')
      .then(res => res.json())
      .then(data => setSettings(data))
      .catch(() => {})
  }, [])

  const handleSave = async () => {
    setLoading(true)
    setStatus('idle')
    
    try {
      const response = await fetch('/api/device/settings', {
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
          <h1 className="text-3xl font-bold mb-2">Device Settings</h1>
          <p className="text-gray-600 dark:text-gray-400">
            Configure device name, mode, power management, and display
          </p>
        </div>

        <div className="card p-8">
          <div className="space-y-6">
            <div>
              <label className="block text-sm font-medium mb-2">Device Name</label>
              <input
                type="text"
                value={settings.name}
                onChange={(e) => setSettings({ ...settings, name: e.target.value })}
                placeholder="Enter device name"
                className="input"
                maxLength={32}
              />
            </div>

            <div>
              <label className="block text-sm font-medium mb-2">Operation Mode</label>
              <div className="grid grid-cols-2 gap-4">
                <button
                  onClick={() => setSettings({ ...settings, mode: 'presenter' })}
                  className={`p-4 rounded-lg border-2 ${settings.mode === 'presenter' ? 'border-primary-500 bg-primary-50 dark:bg-primary-900/20' : 'border-gray-300 dark:border-gray-600'}`}
                >
                  <h3 className="font-semibold">Presenter</h3>
                  <p className="text-sm text-gray-600 dark:text-gray-400">Send commands</p>
                </button>
                <button
                  onClick={() => setSettings({ ...settings, mode: 'pc' })}
                  className={`p-4 rounded-lg border-2 ${settings.mode === 'pc' ? 'border-primary-500 bg-primary-50 dark:bg-primary-900/20' : 'border-gray-300 dark:border-gray-600'}`}
                >
                  <h3 className="font-semibold">PC Receiver</h3>
                  <p className="text-sm text-gray-600 dark:text-gray-400">Receive commands</p>
                </button>
              </div>
            </div>

            <div>
              <label className="block text-sm font-medium mb-2">Display Brightness: {settings.brightness}</label>
              <input
                type="range"
                min="0"
                max="255"
                value={settings.brightness}
                onChange={(e) => setSettings({ ...settings, brightness: parseInt(e.target.value) })}
                className="w-full"
              />
            </div>

            <div>
              <label className="flex items-center space-x-2">
                <input
                  type="checkbox"
                  checked={settings.autoSleep}
                  onChange={(e) => setSettings({ ...settings, autoSleep: e.target.checked })}
                  className="rounded"
                />
                <span className="text-sm font-medium">Enable Auto Sleep</span>
              </label>
            </div>

            <div>
              <label className="block text-sm font-medium mb-2">Sleep Timeout (seconds)</label>
              <input
                type="number"
                value={settings.sleepTimeout / 1000}
                onChange={(e) => setSettings({ ...settings, sleepTimeout: parseInt(e.target.value) * 1000 })}
                className="input"
                min="10"
                max="3600"
              />
            </div>

            <div className="flex items-center justify-between pt-4 border-t border-gray-200 dark:border-gray-700">
              <div className="flex items-center space-x-2">
                {status === 'success' && (
                  <>
                    <CheckCircle className="text-green-500" size={20} />
                    <span className="text-green-600">Settings saved</span>
                  </>
                )}
                {status === 'error' && (
                  <>
                    <AlertCircle className="text-red-500" size={20} />
                    <span className="text-red-600">Failed to save</span>
                  </>
                )}
              </div>
              <button
                onClick={handleSave}
                disabled={loading}
                className="btn-primary flex items-center space-x-2"
              >
                {loading ? <Loader2 className="animate-spin" size={18} /> : <Save size={18} />}
                <span>{loading ? 'Saving...' : 'Save'}</span>
              </button>
            </div>
          </div>
        </div>
      </div>
    </Layout>
  )
}
