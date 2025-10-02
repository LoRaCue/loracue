import { useState, useEffect } from 'react'
import Layout from '../components/Layout'
import { Trash2, Plus, Loader2, CheckCircle, AlertCircle } from 'lucide-react'

interface Device {
  id: number
  name: string
  mac: string
  active: boolean
}

export default function DevicesPage() {
  const [devices, setDevices] = useState<Device[]>([])
  const [loading, setLoading] = useState(true)
  const [status, setStatus] = useState<'idle' | 'success' | 'error'>('idle')

  const loadDevices = async () => {
    try {
      const res = await fetch('/api/devices')
      const data = await res.json()
      setDevices(data)
    } catch (error) {
      console.error('Failed to load devices:', error)
    } finally {
      setLoading(false)
    }
  }

  useEffect(() => {
    loadDevices()
  }, [])

  const handleDelete = async (id: number) => {
    if (!confirm('Remove this device?')) return
    
    try {
      const res = await fetch(`/api/devices/${id}`, { method: 'DELETE' })
      if (res.ok) {
        setStatus('success')
        loadDevices()
        setTimeout(() => setStatus('idle'), 3000)
      } else {
        setStatus('error')
      }
    } catch (error) {
      setStatus('error')
    }
  }

  return (
    <Layout>
      <div className="space-y-8">
        <div className="flex justify-between items-center">
          <div>
            <h1 className="text-3xl font-bold mb-2">Paired Devices</h1>
            <p className="text-gray-600 dark:text-gray-400">
              Manage devices paired with this receiver
            </p>
          </div>
        </div>

        {status === 'success' && (
          <div className="flex items-center space-x-2 text-green-600 bg-green-50 dark:bg-green-900/20 p-4 rounded-lg">
            <CheckCircle size={20} />
            <span>Device removed successfully</span>
          </div>
        )}

        {status === 'error' && (
          <div className="flex items-center space-x-2 text-red-600 bg-red-50 dark:bg-red-900/20 p-4 rounded-lg">
            <AlertCircle size={20} />
            <span>Failed to remove device</span>
          </div>
        )}

        <div className="card">
          {loading ? (
            <div className="flex justify-center items-center p-12">
              <Loader2 className="animate-spin" size={32} />
            </div>
          ) : devices.length === 0 ? (
            <div className="text-center p-12">
              <p className="text-gray-500 mb-4">No devices paired yet</p>
              <p className="text-sm text-gray-400">Use USB pairing on the device to add presenters</p>
            </div>
          ) : (
            <div className="divide-y divide-gray-200 dark:divide-gray-700">
              {devices.map((device) => (
                <div key={device.id} className="p-6 flex items-center justify-between hover:bg-gray-50 dark:hover:bg-gray-800/50">
                  <div className="flex-1">
                    <div className="flex items-center space-x-3">
                      <h3 className="font-semibold text-lg">{device.name}</h3>
                      {device.active && (
                        <span className="px-2 py-1 text-xs bg-green-100 dark:bg-green-900/30 text-green-700 dark:text-green-400 rounded-full">
                          Active
                        </span>
                      )}
                    </div>
                    <p className="text-sm text-gray-500 mt-1">
                      MAC: {device.mac} • ID: 0x{device.id.toString(16).toUpperCase().padStart(4, '0')}
                    </p>
                  </div>
                  <button
                    onClick={() => handleDelete(device.id)}
                    className="p-2 text-red-600 hover:bg-red-50 dark:hover:bg-red-900/20 rounded-lg transition-colors"
                    title="Remove device"
                  >
                    <Trash2 size={20} />
                  </button>
                </div>
              ))}
            </div>
          )}
        </div>

        <div className="card p-6 bg-blue-50 dark:bg-blue-900/20 border-blue-200 dark:border-blue-800">
          <h3 className="font-semibold mb-2">How to pair a device</h3>
          <ol className="text-sm text-gray-700 dark:text-gray-300 space-y-1 list-decimal list-inside">
            <li>On the presenter device, go to Settings → Device Pairing</li>
            <li>Connect presenter to this PC via USB</li>
            <li>Press both buttons to initiate pairing</li>
            <li>Device will appear in the list above</li>
          </ol>
        </div>
      </div>
    </Layout>
  )
}
