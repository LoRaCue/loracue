# Manual Setup Steps for Secure Release Workflow

This document outlines the remaining manual steps to complete the secure firmware release workflow implementation.

## 📋 Overview

**Status**: Phase 1 implementation complete ✅  
**Remaining**: Manual GitHub and Vercel configuration

## 🔐 Step 1: Configure GitHub Secrets (LoRaCue/loracue)

### 1.1 Add FIRMWARE_SIGNING_KEY

```bash
# Location of private key
/Users/fabian/Source/LoRaCue/keys/firmware_private.pem

# Steps:
1. Go to https://github.com/LoRaCue/loracue/settings/secrets/actions
2. Click "New repository secret"
3. Name: FIRMWARE_SIGNING_KEY
4. Value: (paste entire content of keys/firmware_private.pem)
5. Click "Add secret"
```

**⚠️ IMPORTANT**: Never commit `firmware_private.pem` to repository!

### 1.2 Create Personal Access Token for Webhook

```bash
# Create PAT:
1. Go to https://github.com/settings/tokens/new
2. Note: "LoRaCue Release Webhook"
3. Expiration: No expiration (or 1 year with renewal)
4. Scopes:
   ✓ repo (all)
5. Click "Generate token"
6. Copy token (you won't see it again!)

# Add to loracue repository:
1. Go to https://github.com/LoRaCue/loracue/settings/secrets/actions
2. Click "New repository secret"
3. Name: RELEASE_REPO_TOKEN
4. Value: (paste PAT)
5. Click "Add secret"
```

## 🏗️ Step 2: Create release.loracue.de Repository

### 2.1 Create Private Repository

```bash
# On GitHub:
1. Go to https://github.com/organizations/LoRaCue/repositories/new
2. Repository name: release.loracue.de
3. Description: "Private repository for LoRaCue firmware release index"
4. Visibility: ⚠️ Private
5. Initialize: ❌ Do NOT initialize (we have files ready)
6. Click "Create repository"
```

### 2.2 Push Files to Repository

```bash
# From terminal:
cd /tmp/release.loracue.de

git init
git add .
git commit -m "chore: initial repository setup

- Add release index generation script
- Add GitHub Actions workflow for auto-updates
- Add Vercel configuration
- Add documentation and setup guide"

git branch -M main
git remote add origin git@github.com:LoRaCue/release.loracue.de.git
git push -u origin main
```

### 2.3 Configure GitHub Secrets

```bash
# Add FIRMWARE_SIGNING_KEY:
1. Go to https://github.com/LoRaCue/release.loracue.de/settings/secrets/actions
2. Click "New repository secret"
3. Name: FIRMWARE_SIGNING_KEY
4. Value: (paste same private key from loracue repo)
5. Click "Add secret"
```

## 🌐 Step 3: Deploy to Vercel

### 3.1 Connect Repository to Vercel

```bash
# On Vercel Dashboard:
1. Go to https://vercel.com/new
2. Click "Import Git Repository"
3. Select "LoRaCue/release.loracue.de"
4. Configure Project:
   - Framework Preset: Other
   - Root Directory: ./
   - Build Command: (leave empty)
   - Output Directory: public
   - Install Command: (leave empty)
5. Click "Deploy"
6. Wait for deployment to complete
```

### 3.2 Configure Custom Domain

```bash
# On Vercel Project Settings:
1. Go to project → Settings → Domains
2. Add domain: release.loracue.de
3. Vercel will show DNS configuration:
   
   Type: CNAME
   Name: release
   Value: cname.vercel-dns.com
   
4. Add this CNAME record to your DNS provider
5. Wait for DNS propagation (5-30 minutes)
6. Verify HTTPS certificate is issued automatically
```

### 3.3 Verify Deployment

```bash
# Test endpoints:
curl https://release.loracue.de/releases.json
curl https://release-loracue-de.vercel.app/releases.json

# Expected response:
{
  "schema_version": "1.0.0",
  "generated_at": "2025-10-22T16:37:00Z",
  "latest_stable": null,
  "latest_prerelease": null,
  "releases": [],
  "signature": ""
}
```

## ✅ Step 4: Test Complete Workflow

### 4.1 Trigger Manual Workflow

```bash
# Test release index generation:
1. Go to https://github.com/LoRaCue/release.loracue.de/actions
2. Select "Update Release Index" workflow
3. Click "Run workflow"
4. Wait for completion
5. Check that releases.json is updated with existing releases
```

### 4.2 Create Test Release

```bash
# From loracue repository:
cd /Users/fabian/Source/LoRaCue

# Create and push a test tag:
git tag -a v0.2.0-test.1 -m "Test release for secure workflow"
git push origin v0.2.0-test.1

# Monitor workflows:
1. Watch release.yml in loracue repo
2. Verify binaries are signed
3. Verify manifest.json is generated
4. Verify webhook triggers release.loracue.de workflow
5. Verify releases.json is updated
6. Check Vercel auto-deploys
```

### 4.3 Verify Signatures

```bash
# Download and verify manifest signature:
curl -L https://github.com/LoRaCue/loracue/releases/download/v0.2.0-test.1/manifest.json > manifest.json

python3 << 'EOF'
import json
import base64
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.backends import default_backend

with open('manifest.json') as f:
    manifest = json.load(f)

signature = manifest.pop('signature')
manifest_json = json.dumps(manifest, indent=2, sort_keys=True)

with open('keys/firmware_public.pem', 'rb') as f:
    public_key = serialization.load_pem_public_key(f.read(), backend=default_backend())

try:
    public_key.verify(
        base64.b64decode(signature),
        manifest_json.encode('utf-8'),
        padding.PKCS1v15(),
        hashes.SHA256()
    )
    print("✓ Manifest signature valid")
except Exception as e:
    print(f"✗ Manifest signature invalid: {e}")
EOF
```

## 📊 Step 5: Monitoring and Verification

### 5.1 Check Workflow Runs

```bash
# LoRaCue/loracue workflows:
https://github.com/LoRaCue/loracue/actions

# LoRaCue/release.loracue.de workflows:
https://github.com/LoRaCue/release.loracue.de/actions

# Vercel deployments:
https://vercel.com/loracue/release-loracue-de
```

### 5.2 Verify Release Assets

```bash
# For each release, verify these files exist:
- heltec_v3.bin
- heltec_v3.bin.sig
- heltec_v3.bin.sha256
- bootloader.bin
- bootloader.bin.sig
- bootloader.bin.sha256
- partition-table.bin
- partition-table.bin.sig
- partition-table.bin.sha256
- manifest.json
- README.md
```

### 5.3 Test CORS and Caching

```bash
# Verify CORS headers:
curl -I https://release.loracue.de/releases.json | grep -i "access-control"
# Expected: Access-Control-Allow-Origin: *

# Verify cache headers:
curl -I https://release.loracue.de/releases.json | grep -i "cache-control"
# Expected: Cache-Control: public, max-age=300, s-maxage=300
```

## 🔒 Security Checklist

- [ ] Private key stored only in GitHub Secrets (both repos)
- [ ] Private key NOT committed to any repository
- [ ] Public key committed to both repositories
- [ ] RELEASE_REPO_TOKEN has minimal required permissions
- [ ] release.loracue.de repository is private
- [ ] Vercel deployment is public (only serves releases.json)
- [ ] All signatures verify correctly
- [ ] HTTPS enabled on custom domain
- [ ] DNS configured correctly

## 🎯 Success Criteria

✅ **Workflow is complete when:**

1. GitHub Secrets configured in both repositories
2. release.loracue.de repository created and pushed
3. Vercel deployment live at https://release.loracue.de
4. Custom domain configured with HTTPS
5. Test release creates signed binaries
6. Webhook triggers release index update
7. releases.json accessible via HTTPS
8. All signatures verify correctly
9. CORS and cache headers present
10. Documentation updated

## 📚 Reference Documentation

- **Setup Guide**: `/tmp/release.loracue.de/SETUP.md`
- **Architecture**: `docs/RELEASE_WORKFLOW.md`
- **Workflow Details**: `.github/workflows/release.yml`
- **Index Generation**: `/tmp/release.loracue.de/scripts/generate_releases_index.py`

## 🆘 Troubleshooting

### Issue: Workflow fails with "FIRMWARE_SIGNING_KEY not found"

**Solution**: Add secret to GitHub repository settings

### Issue: Webhook doesn't trigger

**Solution**: 
1. Verify RELEASE_REPO_TOKEN has `repo` scope
2. Check loracue workflow logs for webhook errors
3. Manually trigger release.loracue.de workflow

### Issue: Signature verification fails

**Solution**:
1. Verify public and private keys match
2. Check that private key includes BEGIN/END markers
3. Ensure no extra whitespace in secret value

### Issue: Vercel deployment fails

**Solution**:
1. Verify public/ directory exists
2. Check vercel.json syntax
3. Ensure no build command configured

### Issue: Domain not accessible

**Solution**:
1. Wait for DNS propagation (up to 48 hours)
2. Verify CNAME record points to cname.vercel-dns.com
3. Check Vercel domain settings

## 📞 Support

For issues or questions:
- **GitHub Issues**: https://github.com/LoRaCue/loracue/issues
- **Documentation**: https://github.com/LoRaCue/loracue/wiki
- **Workflow Logs**: Check GitHub Actions for detailed error messages

---

**Last Updated**: 2025-10-22  
**Status**: Ready for manual setup  
**Estimated Time**: 30-45 minutes
