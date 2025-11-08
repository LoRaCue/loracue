# GitHub Actions Setup

## Required Secrets

### IDF_COMPONENT_API_TOKEN

This token is required for publishing the ESP-IDF component to the component registry.

**Setup:**

1. Go to repository Settings → Secrets and variables → Actions
2. Click "New repository secret"
3. Name: `IDF_COMPONENT_API_TOKEN`
4. Value: Your ESP Component Registry API token
5. Click "Add secret"

**Getting the API Token:**

1. Visit https://components.espressif.com/
2. Sign in with your account
3. Go to your profile settings
4. Generate or copy your API token

## Workflows

### CI Workflow (`ci.yml`)

**Triggers:**
- Push to `main` branch
- Pull requests to `main` branch

**Jobs:**
- Build I2Console firmware (RP2350)
- Build ESP-IDF example
- Validate ESP-IDF component manifest

### Release Workflow (`release.yml`)

**Triggers:**
- Push tags with `v` prefix (e.g., `v0.1.0`)
- Manual workflow dispatch with version input

**Jobs:**
1. Create tag (manual trigger only)
2. Build firmware and create GitHub release
3. Publish ESP-IDF component to registry

**Manual Release:**

1. Go to Actions → Release → Run workflow
2. Enter version (e.g., `0.1.0` without `v` prefix)
3. Click "Run workflow"

This will:
- Create tag `v0.1.0`
- Build firmware
- Create GitHub release with artifacts
- Publish component to ESP registry

**Tag-based Release:**

```bash
git tag v0.1.0
git push origin v0.1.0
```

This automatically triggers the release workflow.

## Security Notes

⚠️ **Never commit API tokens directly to the repository!**

The token is stored securely in GitHub Secrets and only accessible to workflows.
