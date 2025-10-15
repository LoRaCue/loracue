import { useState, useEffect } from 'react'
import Layout from '../components/Layout'
import { useToast } from '../components/Toast'
import { Trash2, Plus, Edit2, X, Shuffle, Copy } from 'lucide-react'

interface Device {
  name: string
  mac: string
  aes_key: string
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
  const [formData, setFormData] = useState({ name: '', mac: '', aes_key: '' })
  const [copied, setCopied] = useState(false)

  const handleCopy = async () => {
    await navigator.clipboard.writeText(formData.aes_key)
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
    setFormData({ name: '', mac: '', aes_key: '' })
    setShowModal(true)
  }

  const handleEdit = (device: Device) => {
    setEditDevice(device)
    // In edit mode, show MAC but don't include aes_key (user must provide new one)
    setFormData({ name: device.name, mac: device.mac, aes_key: '' })
    setShowModal(true)
  }

  const handleDelete = async (mac: string) => {
    if (!confirm('Remove this device?')) return
    
    try {
      // DELETE /api/devices/* expects JSON body with mac
      await fetch(`/api/devices/${encodeURIComponent(mac)}`, { 
        method: 'DELETE',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ mac })
      })
      toast.success('Device removed')
      loadDevices()
    } catch (error) {
      toast.error('Failed to delete device')
      console.error('Failed to delete device:', error)
    }
  }

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault()
    
    try {
      // POST /api/devices handles both PAIR_DEVICE (add) and UPDATE_PAIRED_DEVICE (edit)
      const command = editDevice ? 'UPDATE_PAIRED_DEVICE' : 'PAIR_DEVICE'
      const payload = {
        name: formData.name,
        mac: formData.mac,
        aes_key: formData.aes_key
      }
      
      await fetch('/api/devices', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      })
      
      toast.success(editDevice ? 'Device updated' : 'Device added')
      setShowModal(false)
      loadDevices()
    } catch (error) {
      toast.error('Failed to save device')
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
      <div className="space-y-4 sm:space-y-6">
        <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-4">
          <div>
            <h1 className="text-2xl sm:text-3xl font-bold">Paired Devices</h1>
            <p className="text-sm sm:text-base text-gray-500 dark:text-gray-400 mt-1 sm:mt-2">
              Manage devices that can control this presenter
            </p>
          </div>
          <button onClick={handleAdd} className="btn-primary flex items-center justify-center space-x-2 w-full sm:w-auto">
            <Plus className="w-4 h-4" />
            <span>Add Device</span>
          </button>
        </div>

        {devices.length === 0 ? (
          <div className="card p-6 sm:p-8 text-center">
            <p className="text-gray-500 dark:text-gray-400">No devices paired yet</p>
            <button onClick={handleAdd} className="btn-primary mt-4 inline-flex items-center space-x-2">
              <Plus className="w-4 h-4" />
              <span>Add First Device</span>
            </button>
          </div>
        ) : (
          <div className="grid gap-3 sm:gap-4">
            {devices.map((device) => (
              <div key={device.mac} className="card p-4 sm:p-6 flex flex-col sm:flex-row sm:items-center sm:justify-between gap-3">
                <div className="min-w-0 flex-1">
                  <h3 className="font-semibold text-base sm:text-lg truncate">{device.name}</h3>
                  <p className="text-xs sm:text-sm text-gray-500 dark:text-gray-400 font-mono break-all sm:break-normal">{device.mac}</p>
                </div>
                <div className="flex items-center space-x-2 self-end sm:self-auto">
                  <button
                    onClick={() => handleEdit(device)}
                    className="p-2 text-blue-600 hover:bg-blue-50 dark:hover:bg-blue-900/20 rounded-lg transition-colors"
                    title="Edit device"
                  >
                    <Edit2 className="w-5 h-5" />
                  </button>
                  <button
                    onClick={() => handleDelete(device.mac)}
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
            <div className="bg-white dark:bg-gray-800 rounded-lg shadow-xl max-w-2xl w-full max-h-[90vh] overflow-y-auto">
              <div className="sticky top-0 bg-white dark:bg-gray-800 border-b border-gray-200 dark:border-gray-700 p-4 sm:p-6">
                <div className="flex items-center justify-between">
                  <h2 className="text-lg sm:text-xl font-bold">
                    {editDevice ? 'Edit Device' : 'Add Device'}
                  </h2>
                  <button
                    onClick={() => setShowModal(false)}
                    className="p-1 hover:bg-gray-100 dark:hover:bg-gray-700 rounded"
                  >
                    <X className="w-5 h-5" />
                  </button>
                </div>
              </div>

              <form onSubmit={handleSubmit} className="p-4 sm:p-6 space-y-4">
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
                  <label className="block text-sm font-medium mb-2">
                    MAC Address {editDevice && <span className="text-xs text-gray-500">(read-only)</span>}
                  </label>
                  <input
                    type="text"
                    value={formData.mac}
                    onChange={(e) => !editDevice && handleMacChange(e.target.value)}
                    className="input w-full font-mono text-sm"
                    placeholder="aa:bb:cc:dd:ee:ff"
                    pattern="[0-9a-fA-F]{2}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}"
                    readOnly={!!editDevice}
                    disabled={!!editDevice}
                    required
                  />
                  {editDevice && (
                    <p className="text-xs text-gray-500 dark:text-gray-400 mt-1">
                      MAC address cannot be changed. To change MAC, delete and add new device.
                    </p>
                  )}
                </div>

                <div>
                  <label className="block text-sm font-medium mb-2">
                    AES-256 Key (64 hex chars) {editDevice && <span className="text-xs text-orange-600 dark:text-orange-400">*required for update</span>}
                  </label>
                  <div className="flex flex-col sm:flex-row gap-2">
                    <input
                      type="text"
                      value={formData.aes_key}
                      onChange={(e) => setFormData({ ...formData, aes_key: e.target.value })}
                      className="input flex-1 font-mono text-xs sm:text-sm"
                      placeholder="0123456789abcdef..."
                      pattern="[0-9a-fA-F]{64}"
                      required
                    />
                    <div className="flex gap-2">
                      <button
                        type="button"
                        onClick={() => setFormData({ ...formData, aes_key: generateRandomKey() })}
                        className="flex-1 sm:flex-none px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-lg hover:bg-gray-50 dark:hover:bg-gray-700 transition-colors"
                        title="Generate random key"
                      >
                        <Shuffle className="w-5 h-5 mx-auto" />
                      </button>
                      <button
                        type="button"
                        onClick={handleCopy}
                        className={`flex-1 sm:flex-none px-3 py-2 border rounded-lg transition-colors ${
                          copied 
                            ? 'border-green-500 bg-green-50 dark:bg-green-900/20 text-green-600 dark:text-green-400'
                            : 'border-gray-300 dark:border-gray-600 hover:bg-gray-50 dark:hover:bg-gray-700'
                        }`}
                        title={copied ? "Copied!" : "Copy to clipboard"}
                        disabled={!formData.aes_key}
                      >
                        <Copy className="w-5 h-5 mx-auto" />
                      </button>
                    </div>
                  </div>
                  <p className="text-xs text-gray-500 dark:text-gray-400 mt-1">
                    32-byte encryption key in hexadecimal
                  </p>
                </div>

                <div className="flex flex-col sm:flex-row gap-3 pt-4">
                  <button
                    type="button"
                    onClick={() => setShowModal(false)}
                    className="order-2 sm:order-1 flex-1 px-4 py-2 border border-gray-300 dark:border-gray-600 rounded-lg hover:bg-gray-50 dark:hover:bg-gray-700 transition-colors"
                  >
                    Cancel
                  </button>
                  <button type="submit" className="order-1 sm:order-2 flex-1 btn-primary">
                    {editDevice ? 'Update Device' : 'Add Device'}
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
