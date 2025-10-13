import { useState, useEffect } from 'react'
import Layout from '../components/Layout'
import { useToast } from '../components/Toast'
import { RefreshCw, AlertTriangle, Loader2 } from 'lucide-react'

interface SystemInfo {
  version: string
  board_id: string
  commit: string
  branch: string
  build_date: string
  chip_model: string
  chip_revision: number
  cpu_cores: number
  flash_size_mb: number
  mac: string
  uptime_sec: number
  free_heap_kb: number
  partition: string
}

export default function SystemPage() {
  const toast = useToast()
  const [info, setInfo] = useState<SystemInfo | null>(null)
  const [loading, setLoading] = useState(true)
  const [resetting, setResetting] = useState(false)

  const loadInfo = async () => {
    try {
      const res = await fetch('/api/device/info')
      if (!res.ok) return
      const data = await res.json()
      setInfo(data)
    } catch (error) {
      // Silently fail
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
      toast.success('Device is resetting. Please wait 10 seconds and reconnect.')
    } catch (error) {
      toast.success('Reset initiated. Device will reboot.')
    }
  }

  const formatUptime = (seconds: number) => {
    const days = Math.floor(seconds / 86400)
    const hours = Math.floor((seconds % 86400) / 3600)
    const mins = Math.floor((seconds % 3600) / 60)
    return `${days}d ${hours}h ${mins}m`
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
                  <dt className="text-sm text-gray-500 dark:text-gray-400">Board ID</dt>
                  <dd className="font-mono">{info.board_id}</dd>
                </div>
                <div>
                  <dt className="text-sm text-gray-500 dark:text-gray-400">Version</dt>
                  <dd className="font-mono">{info.version}</dd>
                </div>
                <div>
                  <dt className="text-sm text-gray-500 dark:text-gray-400">Git Commit</dt>
                  <dd className="font-mono text-sm">{info.commit}</dd>
                </div>
                <div>
                  <dt className="text-sm text-gray-500 dark:text-gray-400">Branch</dt>
                  <dd className="font-mono">{info.branch}</dd>
                </div>
                <div>
                  <dt className="text-sm text-gray-500 dark:text-gray-400">Build Date</dt>
                  <dd className="text-sm">{info.build_date}</dd>
                </div>
                <div>
                  <dt className="text-sm text-gray-500 dark:text-gray-400">Partition</dt>
                  <dd className="font-mono">{info.partition}</dd>
                </div>
              </dl>
            </div>

            <div className="card p-6">
              <h2 className="text-lg font-semibold mb-4">Hardware</h2>
              <dl className="space-y-3">
                <div>
                  <dt className="text-sm text-gray-500 dark:text-gray-400">Chip Model</dt>
                  <dd className="font-mono">{info.chip_model}</dd>
                </div>
                <div>
                  <dt className="text-sm text-gray-500 dark:text-gray-400">Chip Revision</dt>
                  <dd>{info.chip_revision}</dd>
                </div>
                <div>
                  <dt className="text-sm text-gray-500 dark:text-gray-400">CPU Cores</dt>
                  <dd>{info.cpu_cores}</dd>
                </div>
                <div>
                  <dt className="text-sm text-gray-500 dark:text-gray-400">Flash Size</dt>
                  <dd>{info.flash_size_mb} MB</dd>
                </div>
                <div>
                  <dt className="text-sm text-gray-500 dark:text-gray-400">MAC Address</dt>
                  <dd className="font-mono text-sm">{info.mac}</dd>
                </div>
              </dl>
            </div>

            <div className="card p-6">
              <h2 className="text-lg font-semibold mb-4">Runtime</h2>
              <dl className="space-y-3">
                <div>
                  <dt className="text-sm text-gray-500 dark:text-gray-400">Uptime</dt>
                  <dd>{formatUptime(info.uptime_sec)}</dd>
                </div>
                <div>
                  <dt className="text-sm text-gray-500 dark:text-gray-400">Free Heap</dt>
                  <dd>{info.free_heap_kb} KB</dd>
                </div>
              </dl>
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
