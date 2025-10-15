import { Moon, Sun, Wifi, Settings, Upload, Users, Info, Battery, Heart, Menu, X } from 'lucide-react'
import { useTheme } from './ThemeProvider'
import { useDevice } from './DeviceContext'
import Link from 'next/link'
import { useRouter } from 'next/router'
import Logo from './Logo'
import { useState } from 'react'

export default function Layout({ children }: { children: React.ReactNode }) {
  const { theme, toggleTheme } = useTheme()
  const router = useRouter()
  const { deviceName, firmwareVersion, loading } = useDevice()
  const [mobileMenuOpen, setMobileMenuOpen] = useState(false)

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
        <div className="max-w-6xl mx-auto px-4 sm:px-6 py-3 sm:py-4">
          <div className="flex items-center justify-between">
            {/* Logo */}
            <a href="https://github.com/LoRaCue" target="_blank" rel="noopener noreferrer" className="flex-shrink-0">
              <Logo className="w-16 h-16 sm:w-20 sm:h-20 text-primary-600 dark:text-white hover:opacity-80 transition-opacity" />
            </a>

            {/* Desktop Navigation */}
            <div className="hidden lg:flex space-x-1">
              {navigation.map((item) => {
                const Icon = item.icon
                const isActive = router.pathname === item.href
                return (
                  <Link
                    key={item.name}
                    href={item.href}
                    className={`flex items-center space-x-2 px-3 py-2 rounded-lg transition-all duration-200 ${
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

            {/* Right side buttons */}
            <div className="flex items-center space-x-2">
              <button
                onClick={toggleTheme}
                className="p-2 rounded-lg hover:bg-gray-100 dark:hover:bg-gray-800 transition-colors duration-200"
                aria-label="Toggle theme"
              >
                {theme === 'light' ? <Moon size={20} /> : <Sun size={20} />}
              </button>
              
              {/* Mobile menu button */}
              <button
                onClick={() => setMobileMenuOpen(!mobileMenuOpen)}
                className="lg:hidden p-2 rounded-lg hover:bg-gray-100 dark:hover:bg-gray-800 transition-colors duration-200"
                aria-label="Toggle menu"
              >
                {mobileMenuOpen ? <X size={24} /> : <Menu size={24} />}
              </button>
            </div>
          </div>

          {/* Mobile Navigation */}
          {mobileMenuOpen && (
            <div className="lg:hidden mt-3 pb-3 space-y-1">
              {navigation.map((item) => {
                const Icon = item.icon
                const isActive = router.pathname === item.href
                return (
                  <Link
                    key={item.name}
                    href={item.href}
                    onClick={() => setMobileMenuOpen(false)}
                    className={`flex items-center space-x-3 px-4 py-3 rounded-lg transition-all duration-200 ${
                      isActive
                        ? 'bg-primary-100 dark:bg-primary-900/30 text-primary-700 dark:text-primary-300'
                        : 'hover:bg-gray-100 dark:hover:bg-gray-800'
                    }`}
                  >
                    <Icon size={20} />
                    <span className="font-medium">{item.name}</span>
                  </Link>
                )
              })}
            </div>
          )}
        </div>
      </nav>
      
      <main className="max-w-6xl mx-auto px-4 sm:px-6 py-6 sm:py-8">
        {children}
      </main>

      <footer className="max-w-6xl mx-auto px-4 sm:px-6 py-6 text-center text-xs sm:text-sm text-gray-600 dark:text-gray-400">
        <div className="flex flex-col sm:flex-row items-center justify-center gap-2 sm:gap-1 flex-wrap">
          <span>
            <a href="https://github.com/LoRaCue" target="_blank" rel="noopener noreferrer" className="hover:text-primary-600 dark:hover:text-primary-400">LoRaCue</a>
            {' '}is made with <Heart size={14} className="inline text-red-500" fill="currentColor" /> in{' '}
            <a href="https://hannover.de" target="_blank" rel="noopener noreferrer" className="hover:text-primary-600 dark:hover:text-primary-400">Hannover</a>
          </span>
          <span className="hidden sm:inline">•</span>
          <span>Device: {loading ? <span className="inline-block w-20 h-3 bg-gray-300 dark:bg-gray-700 rounded animate-pulse align-middle" /> : deviceName}</span>
          <span className="hidden sm:inline">•</span>
          <span>Firmware: {loading ? <span className="inline-block w-12 h-3 bg-gray-300 dark:bg-gray-700 rounded animate-pulse align-middle" /> : firmwareVersion}</span>
          <span className="hidden sm:inline">•</span>
          <span>
            Licensed under{' '}
            <a href="https://www.gnu.org/licenses/gpl-3.0.html" target="_blank" rel="noopener noreferrer" className="hover:text-primary-600 dark:hover:text-primary-400">GPL-3</a>
          </span>
        </div>
      </footer>
    </div>
  )
}
