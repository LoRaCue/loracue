import '../styles/globals.css'
import type { AppProps } from 'next/app'
import { ThemeProvider } from '../components/ThemeProvider'
import { DeviceProvider } from '../components/DeviceContext'
import { ToastProvider } from '../components/Toast'

export default function App({ Component, pageProps }: AppProps) {
  return (
    <ThemeProvider>
      <DeviceProvider>
        <ToastProvider>
          <Component {...pageProps} />
        </ToastProvider>
      </DeviceProvider>
    </ThemeProvider>
  )
}
