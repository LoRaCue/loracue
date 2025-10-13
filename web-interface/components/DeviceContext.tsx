import { createContext, useContext, useState, useEffect, ReactNode } from 'react'

interface DeviceContextType {
  deviceName: string
  firmwareVersion: string
  loading: boolean
}

const DeviceContext = createContext<DeviceContextType>({ 
  deviceName: '', 
  firmwareVersion: '', 
  loading: true 
})

export const useDevice = () => useContext(DeviceContext)

export function DeviceProvider({ children }: { children: ReactNode }) {
  const [deviceName, setDeviceName] = useState('')
  const [firmwareVersion, setFirmwareVersion] = useState('')
  const [loading, setLoading] = useState(true)

  useEffect(() => {
    Promise.all([
      fetch('/api/general').then(r => r.ok ? r.json() : Promise.reject()).then(d => setDeviceName(d.name)).catch(() => setDeviceName('Unknown')),
      fetch('/api/device/info').then(r => r.ok ? r.json() : Promise.reject()).then(d => setFirmwareVersion(d.commit.substring(0, 7))).catch(() => setFirmwareVersion('Unknown'))
    ]).finally(() => setLoading(false))
  }, [])

  return (
    <DeviceContext.Provider value={{ deviceName, firmwareVersion, loading }}>
      {children}
    </DeviceContext.Provider>
  )
}
