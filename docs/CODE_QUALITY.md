# Code Quality Gates

This project enforces code quality through automated pre-commit hooks and CI checks.

## Pre-Commit Hooks

Git hooks automatically run before each commit to catch issues early:

- **Code Formatting**: Validates C/C++ code style with clang-format
- **Static Analysis**: Detects bugs and code smells with cppcheck
- **Commit Messages**: Enforces conventional commit format

### Setup

```bash
# Install dependencies (one-time setup)
npm install

# Hooks are automatically installed via husky
```

### Requirements

**clang-format** (required):

```bash
# macOS
brew install clang-format

# Ubuntu/Debian
sudo apt install clang-format

# Arch Linux
sudo pacman -S clang
```

**cppcheck** (optional but recommended):

```bash
# macOS
brew install cppcheck

# Ubuntu/Debian
sudo apt install cppcheck

# Arch Linux
sudo pacman -S cppcheck
```

## Manual Commands

```bash
# Check code formatting
make format-check

# Auto-fix formatting issues
make format

# Run static analysis
make lint
```

## Static Analysis

cppcheck detects:
- Memory leaks and null pointer dereferences
- Buffer overflows and array bounds violations
- Unused variables and dead code
- Performance issues
- Portability problems

Suppressing false positives:
```c
// cppcheck-suppress unusedVariable
int temp = 0;
```

## Bypassing Hooks (Not Recommended)

```bash
# Skip pre-commit checks (use only in emergencies)
git commit --no-verify -m "message"
```

## Code Style

We follow LLVM style with customizations:

- **Indent**: 4 spaces
- **Line Length**: 120 characters
- **Braces**: Linux style (opening brace on same line)
- **Pointers**: Right-aligned (`int *ptr`)

See `.clang-format` for full configuration.

## CI Integration

GitHub Actions runs the same checks on every push/PR:

1. Commit message validation
2. Code formatting verification
3. Static analysis
4. Build verification

All checks must pass before merging.
