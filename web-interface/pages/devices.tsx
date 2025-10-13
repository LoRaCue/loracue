import { useState, useEffect } from 'react'
import Layout from '../components/Layout'
import { useToast } from '../components/Toast'
import { Trash2, Plus, Edit2, X, Shuffle, Copy } from 'lucide-react'

interface Device {
  id: number
  name: string
  mac: string
}

const generateRandomKey = (): string => {
  const array = new Uint8Array(32)
  crypto.getRandomValues(array)
  return Array.from(array, byte => byte.toString(16).padStart(2, '0')).join('')
}

export default function DevicesPage() {
  const toast = useToast()
  const [devices, setDevices] = useState<Device[]>([])
  const [loading, setLoading] = useState(true)
  const [showModal, setShowModal] = useState(false)
  const [editDevice, setEditDevice] = useState<Device | null>(null)
  const [formData, setFormData] = useState({ name: '', mac: '', key: '' })
  const [copied, setCopied] = useState(false)

  const handleCopy = async () => {
    await navigator.clipboard.writeText(formData.key)
    setCopied(true)
    setTimeout(() => setCopied(false), 2000)
  }

  const handleMacChange = (value: string) => {
    // Remove all non-hex characters
    const cleaned = value.replace(/[^0-9a-fA-F]/g, '')
    // Add colons every 2 characters
    const formatted = cleaned.match(/.{1,2}/g)?.join(':') || cleaned
    // Limit to 17 characters (12 hex + 5 colons)
    setFormData({ ...formData, mac: formatted.slice(0, 17) })
  }

  const loadDevices = async () => {
    setLoading(true)
    try {
      const res = await fetch('/api/devices')
      if (!res.ok) {
        setDevices([])
        return
      }
      const data = await res.json()
      setDevices(data)
    } catch (error) {
      setDevices([])
    } finally {
      setLoading(false)
    }
  }

  useEffect(() => {
    loadDevices()
  }, [])

  const handleAdd = () => {
    setEditDevice(null)
    setFormData({ name: '', mac: '', key: '' })
    setShowModal(true)
  }

  const handleEdit = (device: Device) => {
    setEditDevice(device)
    setFormData({ name: device.name, mac: device.mac, key: '' })
    setShowModal(true)
  }

  const handleDelete = async (id: number) => {
    if (!confirm('Remove this device?')) return
    
    try {
      await fetch(`/api/devices/${id}`, { method: 'DELETE' })
      loadDevices()
    } catch (error) {
      console.error('Failed to delete device:', error)
    }
  }

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault()
    
    try {
      await fetch('/api/devices', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(formData)
      })
      setShowModal(false)
      loadDevices()
    } catch (error) {
      console.error('Failed to save device:', error)
    }
  }

  if (loading) {
    return (
      <Layout>
        <div className="flex items-center justify-center h-64">
          <div className="text-gray-500">Loading devices...</div>
        </div>
      </Layout>
    )
  }

  return (
    <Layout>
      <div className="space-y-6">
        <div className="flex items-center justify-between">
          <div>
            <h1 className="text-3xl font-bold">Paired Devices</h1>
            <p className="text-gray-500 dark:text-gray-400 mt-2">
              Manage devices that can control this presenter
            </p>
          </div>
          <button onClick={handleAdd} className="btn-primary flex items-center space-x-2">
            <Plus className="w-4 h-4" />
            <span>Add Device</span>
          </button>
        </div>

        {devices.length === 0 ? (
          <div className="card p-8 text-center">
            <p className="text-gray-500 dark:text-gray-400">No devices paired yet</p>
            <button onClick={handleAdd} className="btn-primary mt-4 inline-flex items-center space-x-2">
              <Plus className="w-4 h-4" />
              <span>Add First Device</span>
            </button>
          </div>
        ) : (
          <div className="grid gap-4">
            {devices.map((device) => (
              <div key={device.id} className="card p-6 flex items-center justify-between">
                <div>
                  <h3 className="font-semibold text-lg">{device.name}</h3>
                  <p className="text-sm text-gray-500 dark:text-gray-400 font-mono">{device.mac}</p>
                </div>
                <div className="flex items-center space-x-2">
                  <button
                    onClick={() => handleEdit(device)}
                    className="p-2 text-blue-600 hover:bg-blue-50 dark:hover:bg-blue-900/20 rounded-lg transition-colors"
                    title="Edit device"
                  >
                    <Edit2 className="w-5 h-5" />
                  </button>
                  <button
                    onClick={() => handleDelete(device.id)}
                    className="p-2 text-red-600 hover:bg-red-50 dark:hover:bg-red-900/20 rounded-lg transition-colors"
                    title="Remove device"
                  >
                    <Trash2 className="w-5 h-5" />
                  </button>
                </div>
              </div>
            ))}
          </div>
        )}

        {/* Add/Edit Modal */}
        {showModal && (
          <div className="fixed inset-0 bg-black/50 flex items-center justify-center z-50 p-4">
            <div className="bg-white dark:bg-gray-800 rounded-lg shadow-xl max-w-2xl w-full p-6">
              <div className="flex items-center justify-between mb-4">
                <h2 className="text-xl font-bold">
                  {editDevice ? 'Edit Device' : 'Add Device'}
                </h2>
                <button
                  onClick={() => setShowModal(false)}
                  className="p-1 hover:bg-gray-100 dark:hover:bg-gray-700 rounded"
                >
                  <X className="w-5 h-5" />
                </button>
              </div>

              <form onSubmit={handleSubmit} className="space-y-4">
                <div>
                  <label className="block text-sm font-medium mb-2">Device Name</label>
                  <input
                    type="text"
                    value={formData.name}
                    onChange={(e) => setFormData({ ...formData, name: e.target.value })}
                    className="input w-full"
                    placeholder="My Remote"
                    required
                  />
                </div>

                <div>
                  <label className="block text-sm font-medium mb-2">MAC Address</label>
                  <input
                    type="text"
                    value={formData.mac}
                    onChange={(e) => handleMacChange(e.target.value)}
                    className="input w-full font-mono"
                    placeholder="aa:bb:cc:dd:ee:ff"
                    pattern="[0-9a-fA-F]{2}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}"
                    required
                  />
                </div>

                <div>
                  <label className="block text-sm font-medium mb-2">AES-256 Key (64 hex chars)</label>
                  <div className="flex space-x-2">
                    <input
                      type="text"
                      value={formData.key}
                      onChange={(e) => setFormData({ ...formData, key: e.target.value })}
                      className="input flex-1 font-mono text-sm"
                      placeholder="0123456789abcdef..."
                      pattern="[0-9a-fA-F]{64}"
                      required
                    />
                    <button
                      type="button"
                      onClick={() => setFormData({ ...formData, key: generateRandomKey() })}
                      className="px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-lg hover:bg-gray-50 dark:hover:bg-gray-700 transition-colors"
                      title="Generate random key"
                    >
                      <Shuffle className="w-5 h-5" />
                    </button>
                    <button
                      type="button"
                      onClick={handleCopy}
                      className={`px-3 py-2 border rounded-lg transition-colors ${
                        copied 
                          ? 'border-green-500 bg-green-50 dark:bg-green-900/20 text-green-600 dark:text-green-400'
                          : 'border-gray-300 dark:border-gray-600 hover:bg-gray-50 dark:hover:bg-gray-700'
                      }`}
                      title={copied ? "Copied!" : "Copy to clipboard"}
                      disabled={!formData.key}
                    >
                      <Copy className="w-5 h-5" />
                    </button>
                  </div>
                  <p className="text-xs text-gray-500 dark:text-gray-400 mt-1">
                    32-byte encryption key in hexadecimal
                  </p>
                </div>

                <div className="flex space-x-3 pt-4">
                  <button
                    type="button"
                    onClick={() => setShowModal(false)}
                    className="flex-1 px-4 py-2 border border-gray-300 dark:border-gray-600 rounded-lg hover:bg-gray-50 dark:hover:bg-gray-700 transition-colors"
                  >
                    Cancel
                  </button>
                  <button type="submit" className="flex-1 btn-primary">
                    {editDevice ? 'Update' : 'Add'}
                  </button>
                </div>
              </form>
            </div>
          </div>
        )}
      </div>
    </Layout>
  )
}
