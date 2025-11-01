import '../styles/globals.css'
import type { AppProps } from 'next/app'
import Head from 'next/head'
import { ThemeProvider } from '../components/ThemeProvider'
import { DeviceProvider } from '../components/DeviceContext'
import { ToastProvider } from '../components/Toast'

export default function App({ Component, pageProps }: AppProps) {
  return (
    <>
      <Head>
        <html lang="en" />
        <title>LoRaCue Configuration</title>
      </Head>
      <ThemeProvider>
        <DeviceProvider>
          <ToastProvider>
            <Component {...pageProps} />
          </ToastProvider>
        </DeviceProvider>
      </ThemeProvider>
    </>
  )
}
