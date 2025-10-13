/** @type {import('next').NextConfig} */
const nextConfig = {
  output: 'export',
  trailingSlash: true,
  images: {
    unoptimized: true
  },
  assetPrefix: '',
  basePath: '',
  distDir: 'out',
  devIndicators: {
    buildActivity: false,
    appIsrStatus: false
  }
}

module.exports = nextConfig
