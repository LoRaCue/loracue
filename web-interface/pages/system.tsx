import { useState, useEffect } from 'react'
import Layout from '../components/Layout'
import { RefreshCw, AlertTriangle, Loader2 } from 'lucide-react'

interface SystemInfo {
  version: string
  commit: string
  branch: string
  buildDate: string
  uptime: number
  freeHeap: number
  partition: string
}

export default function SystemPage() {
  const [info, setInfo] = useState<SystemInfo | null>(null)
  const [loading, setLoading] = useState(true)
  const [resetting, setResetting] = useState(false)

  const loadInfo = async () => {
    try {
      const res = await fetch('/api/system/info')
      const data = await res.json()
      setInfo(data)
    } catch (error) {
      console.error('Failed to load system info:', error)
    } finally {
      setLoading(false)
    }
  }

  useEffect(() => {
    loadInfo()
    const interval = setInterval(loadInfo, 5000)
    return () => clearInterval(interval)
  }, [])

  const handleFactoryReset = async () => {
    if (!confirm('Factory reset will erase ALL settings and paired devices. Continue?')) return
    if (!confirm('This action cannot be undone. Are you absolutely sure?')) return
    
    setResetting(true)
    try {
      await fetch('/api/system/factory-reset', { method: 'POST' })
      alert('Device is resetting. Please wait 10 seconds and reconnect.')
    } catch (error) {
      alert('Reset initiated. Device will reboot.')
    }
  }

  const formatUptime = (seconds: number) => {
    const days = Math.floor(seconds / 86400)
    const hours = Math.floor((seconds % 86400) / 3600)
    const mins = Math.floor((seconds % 3600) / 60)
    return `${days}d ${hours}h ${mins}m`
  }

  const formatBytes = (bytes: number) => {
    return `${(bytes / 1024).toFixed(1)} KB`
  }

  return (
    <Layout>
      <div className="space-y-8">
        <div>
          <h1 className="text-3xl font-bold mb-2">System Information</h1>
          <p className="text-gray-600 dark:text-gray-400">
            Device status and maintenance
          </p>
        </div>

        {loading ? (
          <div className="card p-12 flex justify-center">
            <Loader2 className="animate-spin" size={32} />
          </div>
        ) : info && (
          <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
            <div className="card p-6">
              <h2 className="text-lg font-semibold mb-4">Firmware</h2>
              <dl className="space-y-3">
                <div>
                  <dt className="text-sm text-gray-500">Version</dt>
                  <dd className="font-mono">{info.version}</dd>
                </div>
                <div>
                  <dt className="text-sm text-gray-500">Commit</dt>
                  <dd className="font-mono text-sm">{info.commit}</dd>
                </div>
                <div>
                  <dt className="text-sm text-gray-500">Branch</dt>
                  <dd className="font-mono">{info.branch}</dd>
                </div>
                <div>
                  <dt className="text-sm text-gray-500">Build Date</dt>
                  <dd className="text-sm">{info.buildDate}</dd>
                </div>
                <div>
                  <dt className="text-sm text-gray-500">Partition</dt>
                  <dd className="font-mono">{info.partition}</dd>
                </div>
              </dl>
            </div>

            <div className="card p-6">
              <h2 className="text-lg font-semibold mb-4">Runtime</h2>
              <dl className="space-y-3">
                <div>
                  <dt className="text-sm text-gray-500">Uptime</dt>
                  <dd>{formatUptime(info.uptime)}</dd>
                </div>
                <div>
                  <dt className="text-sm text-gray-500">Free Heap</dt>
                  <dd>{formatBytes(info.freeHeap)}</dd>
                </div>
              </dl>
              <button
                onClick={loadInfo}
                className="mt-4 btn-secondary flex items-center space-x-2"
              >
                <RefreshCw size={16} />
                <span>Refresh</span>
              </button>
            </div>
          </div>
        )}

        <div className="card p-6 border-red-200 dark:border-red-800 bg-red-50 dark:bg-red-900/20">
          <div className="flex items-start space-x-3">
            <AlertTriangle className="text-red-600 flex-shrink-0 mt-1" size={24} />
            <div className="flex-1">
              <h3 className="font-semibold text-red-900 dark:text-red-100 mb-2">
                Factory Reset
              </h3>
              <p className="text-sm text-red-800 dark:text-red-200 mb-4">
                This will erase all settings, paired devices, and configuration. 
                The device will reboot with default settings.
              </p>
              <button
                onClick={handleFactoryReset}
                disabled={resetting}
                className="bg-red-600 hover:bg-red-700 text-white px-4 py-2 rounded-lg font-medium disabled:opacity-50 flex items-center space-x-2"
              >
                {resetting && <Loader2 className="animate-spin" size={16} />}
                <span>{resetting ? 'Resetting...' : 'Factory Reset'}</span>
              </button>
            </div>
          </div>
        </div>
      </div>
    </Layout>
  )
}
