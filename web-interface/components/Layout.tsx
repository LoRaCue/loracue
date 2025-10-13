import { Moon, Sun, Wifi, Settings, Upload, Users, Info, Battery, Heart } from 'lucide-react'
import { useTheme } from './ThemeProvider'
import { useDevice } from './DeviceContext'
import Link from 'next/link'
import { useRouter } from 'next/router'
import Logo from './Logo'

export default function Layout({ children }: { children: React.ReactNode }) {
  const { theme, toggleTheme } = useTheme()
  const router = useRouter()
  const { deviceName, firmwareVersion, loading } = useDevice()

  const navigation = [
    { name: 'General', href: '/', icon: Settings },
    { name: 'Power', href: '/power', icon: Battery },
    { name: 'LoRa', href: '/lora', icon: Wifi },
    { name: 'Devices', href: '/devices', icon: Users },
    { name: 'Firmware', href: '/firmware', icon: Upload },
    { name: 'System', href: '/system', icon: Info },
  ]

  return (
    <div className="min-h-screen bg-gradient-to-br from-gray-50 to-gray-100 dark:from-gray-900 dark:to-gray-800">
      <nav className="bg-white/80 dark:bg-gray-900/80 backdrop-blur-md border-b border-gray-200 dark:border-gray-700 sticky top-0 z-50">
        <div className="max-w-4xl mx-auto px-6 py-4">
          <div className="flex items-center justify-between">
            <div className="flex items-center space-x-8">
              <a href="https://github.com/LoRaCue" target="_blank" rel="noopener noreferrer">
                <Logo className="w-24 h-24 text-primary-600 dark:text-white hover:opacity-80 transition-opacity" />
              </a>
              <div className="flex space-x-1">
                {navigation.map((item) => {
                  const Icon = item.icon
                  const isActive = router.pathname === item.href
                  return (
                    <Link
                      key={item.name}
                      href={item.href}
                      className={`flex items-center space-x-2 px-4 py-2 rounded-lg transition-all duration-200 ${
                        isActive
                          ? 'bg-primary-100 dark:bg-primary-900/30 text-primary-700 dark:text-primary-300'
                          : 'hover:bg-gray-100 dark:hover:bg-gray-800'
                      }`}
                    >
                      <Icon size={18} />
                      <span className="font-medium">{item.name}</span>
                    </Link>
                  )
                })}
              </div>
            </div>
            <button
              onClick={toggleTheme}
              className="p-2 rounded-lg hover:bg-gray-100 dark:hover:bg-gray-800 transition-colors duration-200"
            >
              {theme === 'light' ? <Moon size={20} /> : <Sun size={20} />}
            </button>
          </div>
        </div>
      </nav>
      
      <main className="max-w-4xl mx-auto px-6 py-8">
        {children}
      </main>

      <footer className="max-w-4xl mx-auto px-6 py-6 text-center text-sm text-gray-600 dark:text-gray-400">
        <a href="https://github.com/LoRaCue" target="_blank" rel="noopener noreferrer" className="hover:text-primary-600 dark:hover:text-primary-400">LoRaCue</a>
        {' '}is made with <Heart size={14} className="inline text-red-500" fill="currentColor" /> in{' '}
        <a href="https://hannover.de" target="_blank" rel="noopener noreferrer" className="hover:text-primary-600 dark:hover:text-primary-400">Hannover</a>
        {' • '}Device: {loading ? <span className="inline-block w-20 h-3 bg-gray-300 dark:bg-gray-700 rounded animate-pulse" /> : deviceName}
        {' • '}Firmware: {loading ? <span className="inline-block w-12 h-3 bg-gray-300 dark:bg-gray-700 rounded animate-pulse" /> : firmwareVersion}
        {' • '}Licensed under{' '}
        <a href="https://www.gnu.org/licenses/gpl-3.0.html" target="_blank" rel="noopener noreferrer" className="hover:text-primary-600 dark:hover:text-primary-400">GPL-3</a>
      </footer>
    </div>
  )
}
