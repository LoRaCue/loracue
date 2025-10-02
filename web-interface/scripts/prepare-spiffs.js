const fs = require('fs')
const path = require('path')

const sourceDir = path.join(__dirname, '../out')
const targetDir = path.join(__dirname, '../../spiffs_data')

// Create target directory
if (!fs.existsSync(targetDir)) {
  fs.mkdirSync(targetDir, { recursive: true })
}

// Copy files recursively
function copyFiles(src, dest) {
  const items = fs.readdirSync(src)
  
  for (const item of items) {
    const srcPath = path.join(src, item)
    const destPath = path.join(dest, item)
    
    if (fs.statSync(srcPath).isDirectory()) {
      if (!fs.existsSync(destPath)) {
        fs.mkdirSync(destPath, { recursive: true })
      }
      copyFiles(srcPath, destPath)
    } else {
      fs.copyFileSync(srcPath, destPath)
    }
  }
}

// Clean target directory
if (fs.existsSync(targetDir)) {
  fs.rmSync(targetDir, { recursive: true })
}
fs.mkdirSync(targetDir, { recursive: true })

// Copy built files
copyFiles(sourceDir, targetDir)

console.log('✅ Web interface built and copied to spiffs_data/')
console.log(`📁 Files ready for SPIFFS at: ${targetDir}`)
