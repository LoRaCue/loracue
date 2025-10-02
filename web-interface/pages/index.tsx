import { useState, useEffect } from 'react'
import Layout from '../components/Layout'
import { Save, Loader2, CheckCircle, AlertCircle } from 'lucide-react'

interface DeviceSettings {
  name: string
  mode: 'presenter' | 'receiver'
}

export default function DevicePage() {
  const [settings, setSettings] = useState<DeviceSettings>({
    name: '',
    mode: 'presenter'
  })
  const [loading, setLoading] = useState(false)
  const [status, setStatus] = useState<'idle' | 'success' | 'error'>('idle')

  useEffect(() => {
    // Load current settings
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
            Configure your LoRaCue device name and operation mode
          </p>
        </div>

        <div className="card p-8">
          <div className="space-y-6">
            <div>
              <label className="block text-sm font-medium mb-2">
                Device Name
              </label>
              <input
                type="text"
                value={settings.name}
                onChange={(e) => setSettings({ ...settings, name: e.target.value })}
                placeholder="Enter device name"
                className="input"
                maxLength={32}
              />
              <p className="text-sm text-gray-500 mt-1">
                Used for device identification and pairing
              </p>
            </div>

            <div>
              <label className="block text-sm font-medium mb-2">
                Operation Mode
              </label>
              <div className="grid grid-cols-2 gap-4">
                <button
                  onClick={() => setSettings({ ...settings, mode: 'presenter' })}
                  className={`p-4 rounded-lg border-2 transition-all duration-200 ${
                    settings.mode === 'presenter'
                      ? 'border-primary-500 bg-primary-50 dark:bg-primary-900/20'
                      : 'border-gray-300 dark:border-gray-600 hover:border-gray-400 dark:hover:border-gray-500'
                  }`}
                >
                  <div className="text-left">
                    <h3 className="font-semibold">Presenter</h3>
                    <p className="text-sm text-gray-600 dark:text-gray-400 mt-1">
                      Send presentation commands
                    </p>
                  </div>
                </button>
                
                <button
                  onClick={() => setSettings({ ...settings, mode: 'receiver' })}
                  className={`p-4 rounded-lg border-2 transition-all duration-200 ${
                    settings.mode === 'receiver'
                      ? 'border-primary-500 bg-primary-50 dark:bg-primary-900/20'
                      : 'border-gray-300 dark:border-gray-600 hover:border-gray-400 dark:hover:border-gray-500'
                  }`}
                >
                  <div className="text-left">
                    <h3 className="font-semibold">Receiver</h3>
                    <p className="text-sm text-gray-600 dark:text-gray-400 mt-1">
                      Receive and execute commands
                    </p>
                  </div>
                </button>
              </div>
            </div>

            <div className="flex items-center justify-between pt-4 border-t border-gray-200 dark:border-gray-700">
              <div className="flex items-center space-x-2">
                {status === 'success' && (
                  <>
                    <CheckCircle className="text-green-500" size={20} />
                    <span className="text-green-600 dark:text-green-400">Settings saved successfully</span>
                  </>
                )}
                {status === 'error' && (
                  <>
                    <AlertCircle className="text-red-500" size={20} />
                    <span className="text-red-600 dark:text-red-400">Failed to save settings</span>
                  </>
                )}
              </div>
              
              <button
                onClick={handleSave}
                disabled={loading || !settings.name.trim()}
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
