import { Moon, Sun, Radio, Sliders, PackageCheck, Cpu, Server, Zap, Heart, Menu, X, Github } from 'lucide-react'
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
    { name: 'General', href: '/', icon: Sliders },
    { name: 'Power', href: '/power', icon: Zap },
    { name: 'LoRa', href: '/lora', icon: Radio },
    { name: 'Devices', href: '/devices', icon: Cpu },
    { name: 'Firmware', href: '/firmware', icon: PackageCheck },
    { name: 'System', href: '/system', icon: Server },
  ]

  return (
    <div className="min-h-screen bg-gradient-to-br from-gray-200 to-gray-300 dark:from-gray-900 dark:to-gray-800">
      <nav className="bg-white/80 dark:bg-gray-900/80 backdrop-blur-md border-b border-gray-200 dark:border-gray-700 sticky top-0 z-50">
        <div className="max-w-6xl mx-auto px-4 sm:px-6 py-3 sm:py-4">
          <div className="flex items-center justify-between">
            {/* Logo */}
            <a href="https://github.com/LoRaCue" target="_blank" rel="noopener noreferrer" className="flex-shrink-0" aria-label="LoRaCue on GitHub">
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
                    className={`flex items-center space-x-2 px-3 py-2 rounded-lg transition-colors duration-200 ${
                      isActive
                        ? 'bg-primary-50 dark:bg-primary-900/30 text-primary-700 dark:text-primary-300'
                        : 'text-gray-600 dark:text-gray-400 hover:bg-gray-100 dark:hover:bg-gray-800 hover:text-gray-900 dark:hover:text-gray-100'
                    }`}
                  >
                    <Icon size={20} strokeWidth={isActive ? 2.5 : 2} aria-hidden="true" />
                    <span className="font-medium">{item.name}</span>
                  </Link>
                )
              })}
            </div>

            {/* Right side buttons */}
            <div className="flex items-center space-x-2">
              <a
                href="https://github.com/LoRaCue"
                target="_blank"
                rel="noopener noreferrer"
                className="p-2 rounded-lg text-gray-600 dark:text-gray-400 hover:bg-gray-100 dark:hover:bg-gray-800 hover:text-gray-900 dark:hover:text-gray-100 transition-colors duration-200"
                aria-label="View on GitHub"
              >
                <Github size={20} aria-hidden="true" />
              </a>
              <button
                onClick={toggleTheme}
                className="p-2 rounded-lg text-gray-600 dark:text-gray-400 hover:bg-gray-100 dark:hover:bg-gray-800 hover:text-gray-900 dark:hover:text-gray-100 transition-colors duration-200"
                aria-label={theme === 'light' ? 'Switch to dark mode' : 'Switch to light mode'}
              >
                {theme === 'light' ? <Moon size={20} aria-hidden="true" /> : <Sun size={20} aria-hidden="true" />}
              </button>
              
              {/* Mobile menu button */}
              <button
                onClick={() => setMobileMenuOpen(!mobileMenuOpen)}
                className="lg:hidden p-2 rounded-lg text-gray-600 dark:text-gray-400 hover:bg-gray-100 dark:hover:bg-gray-800 hover:text-gray-900 dark:hover:text-gray-100 transition-colors duration-200"
                aria-label={mobileMenuOpen ? 'Close menu' : 'Open menu'}
              >
                {mobileMenuOpen ? <X size={24} aria-hidden="true" /> : <Menu size={24} aria-hidden="true" />}
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
                    className={`flex items-center space-x-3 px-4 py-3 rounded-lg transition-colors duration-200 ${
                      isActive
                        ? 'bg-primary-50 dark:bg-primary-900/30 text-primary-700 dark:text-primary-300'
                        : 'text-gray-600 dark:text-gray-400 hover:bg-gray-100 dark:hover:bg-gray-800 hover:text-gray-900 dark:hover:text-gray-100'
                    }`}
                  >
                    <Icon size={20} strokeWidth={isActive ? 2.5 : 2} aria-hidden="true" />
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
            {' '}is made with <Heart size={14} className="inline text-red-500" fill="currentColor" aria-hidden="true" /> in{' '}
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
