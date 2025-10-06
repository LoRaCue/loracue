import { Moon, Sun, Wifi, Settings, Upload, Users, Info } from 'lucide-react'
import { useTheme } from './ThemeProvider'
import Link from 'next/link'
import { useRouter } from 'next/router'
import Logo from './Logo'

export default function Layout({ children }: { children: React.ReactNode }) {
  const { theme, toggleTheme } = useTheme()
  const router = useRouter()

  const navigation = [
    { name: 'Device', href: '/', icon: Settings },
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
              <Logo className="w-24 h-24 text-primary-600 dark:text-white" />
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
    </div>
  )
}
