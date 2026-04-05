# Build & Usage Guide

## Prerequisites

| Tool | Version |
|------|---------|
| CMake | 3.21+ |
| Conan | 2.x |
| GCC | 11+ (Linux) |
| Clang | 14+ (Linux) |
| MSVC | 2022+ (Windows) |
| Ninja | any recent |
| Python | 3.11+ |

Install Conan via pip:

```bash
pip install "conan>=2,<3"
```

---

## Build & Test

### Linux

```bash
# Install dependencies
conan install . --build=missing -s build_type=Release -s compiler.cppstd=20

# Configure
cmake --preset conan-release

# Build
cmake --build --preset conan-release

# Test
ctest --preset conan-release
```

### Windows (MSVC)

Same commands — run in a **Developer Command Prompt** or with the MSVC environment activated.

```bat
conan install . --build=missing -s build_type=Release -s compiler.cppstd=20
cmake --preset conan-release
cmake --build --preset conan-release
ctest --preset conan-release
```

---

## Install

Produces a self-contained directory under `install/`:

```bash
cmake --build --preset conan-release --target install
```

**Linux layout:**
```
install/
├── bin/challenge
└── lib/
    ├── libplugin.so
    ├── libplugin_segfault.so
    └── libboost_*.so.*
```

**Windows layout:**
```
install/
└── bin/
    ├── challenge.exe
    ├── plugin.dll
    ├── plugin_segfault.dll
    └── boost_*.dll
```

Run the installed binary directly — all shared libraries are found via RPATH (Linux) or `bin/` (Windows):

```bash
./install/bin/challenge
```

---

## Sanitizers (AddressSanitizer)

The debug preset has ASan enabled by default. GCC or Clang only.

```bash
conan install . --build=missing -s build_type=Debug -s compiler.cppstd=20
cmake --preset conan-debug
cmake --build --preset conan-debug
ASAN_OPTIONS=verify_asan_link_order=0 ctest --preset conan-debug
```

To enable ASan manually on any preset:

```bash
cmake --preset conan-release -DENABLE_SANITIZERS=ON
```

---

## Linters

Requires `clang-format`, `clang-tidy`, and `pre-commit` installed:

```bash
pip install pre-commit
pre-commit run --all-files
```

Run individual tools:

```bash
# Format check
clang-format --dry-run --Werror $(find app plugin plugin_segfault tests -name "*.cpp" -o -name "*.h")

# Tidy (needs compile_commands.json from a cmake configure step)
clang-tidy $(find app plugin plugin_segfault tests -name "*.cpp")
```

---

## Conan Package (Task 8)

Build and package the project as a Conan package:

```bash
conan create . --build=missing -s build_type=Release -s compiler.cppstd=20
```

This runs the full build, packages headers and shared libraries into the Conan cache, and runs `test_package/` to validate the package is consumable.

Export the package cache to an archive:

```bash
conan cache save "challenge_project/0.1.0:*" --file challenge-conan-package.tar.gz
```

Restore it on another machine:

```bash
conan cache restore challenge-conan-package.tar.gz
```
