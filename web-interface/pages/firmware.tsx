import { useState, useRef } from 'react'
import Layout from '../components/Layout'
import { Upload, Loader2, CheckCircle, AlertCircle, FileText, Zap } from 'lucide-react'

export default function FirmwarePage() {
  const [file, setFile] = useState<File | null>(null)
  const [uploading, setUploading] = useState(false)
  const [progress, setProgress] = useState(0)
  const [status, setStatus] = useState<'idle' | 'success' | 'error'>('idle')
  const [message, setMessage] = useState('')
  const fileInputRef = useRef<HTMLInputElement>(null)

  const handleFileSelect = (e: React.ChangeEvent<HTMLInputElement>) => {
    const selectedFile = e.target.files?.[0]
    if (selectedFile && selectedFile.name.endsWith('.bin')) {
      setFile(selectedFile)
      setStatus('idle')
    } else {
      setStatus('error')
      setMessage('Please select a valid .bin firmware file')
    }
  }

  const handleUpload = async () => {
    if (!file) return

    setUploading(true)
    setProgress(0)
    setStatus('idle')

    const formData = new FormData()
    formData.append('firmware', file)

    try {
      const xhr = new XMLHttpRequest()
      
      xhr.upload.onprogress = (e) => {
        if (e.lengthComputable) {
          setProgress(Math.round((e.loaded / e.total) * 100))
        }
      }

      xhr.onload = () => {
        if (xhr.status === 200) {
          setStatus('success')
          setMessage('Firmware uploaded successfully! Device will reboot.')
          setFile(null)
          if (fileInputRef.current) fileInputRef.current.value = ''
        } else {
          setStatus('error')
          setMessage('Upload failed. Please try again.')
        }
        setUploading(false)
      }

      xhr.onerror = () => {
        setStatus('error')
        setMessage('Upload failed. Please check your connection.')
        setUploading(false)
      }

      xhr.open('POST', '/api/firmware/upload')
      xhr.send(formData)
    } catch (error) {
      setStatus('error')
      setMessage('Upload failed. Please try again.')
      setUploading(false)
    }
  }

  const formatFileSize = (bytes: number) => {
    if (bytes === 0) return '0 Bytes'
    const k = 1024
    const sizes = ['Bytes', 'KB', 'MB']
    const i = Math.floor(Math.log(bytes) / Math.log(k))
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i]
  }

  return (
    <Layout>
      <div className="space-y-8">
        <div>
          <h1 className="text-3xl font-bold mb-2">Firmware Update</h1>
          <p className="text-gray-600 dark:text-gray-400">
            Upload new firmware to your LoRaCue device
          </p>
        </div>

        <div className="card p-8">
          <div className="space-y-6">
            <div className="border-2 border-dashed border-gray-300 dark:border-gray-600 rounded-lg p-8 text-center">
              <Upload className="mx-auto text-gray-400 mb-4" size={48} />
              <h3 className="text-lg font-medium mb-2">Select Firmware File</h3>
              <p className="text-gray-600 dark:text-gray-400 mb-4">
                Choose a .bin firmware file to upload
              </p>
              <input
                ref={fileInputRef}
                type="file"
                accept=".bin"
                onChange={handleFileSelect}
                className="hidden"
              />
              <button
                onClick={() => fileInputRef.current?.click()}
                className="btn-secondary"
                disabled={uploading}
              >
                Choose File
              </button>
            </div>

            {file && (
              <div className="bg-gray-50 dark:bg-gray-800 rounded-lg p-4">
                <div className="flex items-center space-x-3">
                  <FileText className="text-blue-600 dark:text-blue-400" size={24} />
                  <div className="flex-1">
                    <h4 className="font-medium">{file.name}</h4>
                    <p className="text-sm text-gray-600 dark:text-gray-400">
                      {formatFileSize(file.size)}
                    </p>
                  </div>
                </div>
              </div>
            )}

            {uploading && (
              <div className="space-y-3">
                <div className="flex items-center justify-between">
                  <span className="text-sm font-medium">Uploading firmware...</span>
                  <span className="text-sm text-gray-600 dark:text-gray-400">{progress}%</span>
                </div>
                <div className="w-full bg-gray-200 dark:bg-gray-700 rounded-full h-2">
                  <div
                    className="bg-primary-600 h-2 rounded-full transition-all duration-300"
                    style={{ width: `${progress}%` }}
                  />
                </div>
              </div>
            )}

            {status !== 'idle' && (
              <div className={`flex items-center space-x-2 p-4 rounded-lg ${
                status === 'success' 
                  ? 'bg-green-50 dark:bg-green-900/20 text-green-700 dark:text-green-300'
                  : 'bg-red-50 dark:bg-red-900/20 text-red-700 dark:text-red-300'
              }`}>
                {status === 'success' ? (
                  <CheckCircle size={20} />
                ) : (
                  <AlertCircle size={20} />
                )}
                <span>{message}</span>
              </div>
            )}

            <div className="bg-yellow-50 dark:bg-yellow-900/20 border border-yellow-200 dark:border-yellow-800 rounded-lg p-4">
              <div className="flex items-start space-x-3">
                <Zap className="text-yellow-600 dark:text-yellow-400 mt-0.5" size={20} />
                <div>
                  <h3 className="font-medium text-yellow-900 dark:text-yellow-100">Important Notes</h3>
                  <ul className="text-sm text-yellow-700 dark:text-yellow-300 mt-1 space-y-1">
                    <li>• Do not disconnect the device during upload</li>
                    <li>• The device will automatically reboot after upload</li>
                    <li>• Only upload official LoRaCue firmware files</li>
                    <li>• Backup your settings before updating</li>
                  </ul>
                </div>
              </div>
            </div>

            <div className="flex justify-end pt-4 border-t border-gray-200 dark:border-gray-700">
              <button
                onClick={handleUpload}
                disabled={!file || uploading}
                className="btn-primary disabled:opacity-50 disabled:cursor-not-allowed flex items-center space-x-2"
              >
                {uploading ? <Loader2 className="animate-spin" size={18} /> : <Upload size={18} />}
                <span>{uploading ? 'Uploading...' : 'Upload Firmware'}</span>
              </button>
            </div>
          </div>
        </div>
      </div>
    </Layout>
  )
}
