# Publishing I2Console Component to ESP Component Registry

## Prerequisites

1. Install ESP-IDF Component Manager:
   ```bash
   pip install idf-component-manager
   ```

2. Create account at https://components.espressif.com/

3. Get API token from your profile

## Publishing Steps

### 1. Prepare Component

Ensure all files are in place:
```
components/i2console/
├── idf_component.yml    # Component manifest
├── CMakeLists.txt       # Build configuration
├── Kconfig              # Configuration options
├── LICENSE              # GPL-3.0 license
├── README.md            # Component documentation
├── include/
│   └── i2console.h      # Public API
└── i2console.c          # Implementation
```

### 2. Validate Component

```bash
cd components/i2console
compote component validate
```

### 3. Upload to Registry

```bash
compote component upload --namespace metaneutrons --name i2console
```

Or with API token:
```bash
export IDF_COMPONENT_API_TOKEN=your_token_here
compote component upload --namespace metaneutrons --name i2console
```

### 4. Verify Upload

Visit: https://components.espressif.com/components/metaneutrons/i2console

## Using Published Component

Users can then add it to their projects:

```bash
idf.py add-dependency "metaneutrons/i2console^0.1.0"
```

Or in `idf_component.yml`:
```yaml
dependencies:
  metaneutrons/i2console:
    version: "^0.1.0"
```

## Version Updates

1. Update version in `idf_component.yml`
2. Commit changes
3. Create git tag: `git tag v0.1.1`
4. Upload new version: `compote component upload`

## Documentation

Component registry automatically generates documentation from:
- `README.md` - Main documentation
- `include/*.h` - API reference (Doxygen comments)
- `examples/` - Example code

## Support

- Issues: https://github.com/metaneutrons/I2Console/issues
- Docs: https://docs.espressif.com/projects/idf-component-manager/
