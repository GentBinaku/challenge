
## Quick Overview

You are given a small C++20 project that demonstrates a runtime plugin system.

Your task is to add a new plugin and provide the full build, test, lint, packaging, and CI infrastructure around the existing source code.

## Table of Contents

- [Quick Overview](#quick-overview)
  - [1. Technical Solution Constraints](#1-technical-solution-constraints)
  - [2. Definition of Done](#2-definition-of-done)
  - [3. Technical Interview](#3-technical-interview)
- [Technical Challenge](#technical-challenge)
  - [1. Provided Source Files](#1-provided-source-files)
  - [2. Requirements](#2-requirements)
    - [Task 1 — Use CMake Build System](#task-1--use-cmake-build-system)
    - [Task 2 — Add Dependency Management Using Conan 2](#task-2--add-dependency-management-using-conan-2)
    - [Task 3 — Add a SEGFAULT/Access Violation Detection Plugin](#task-3--add-a-segfaultaccess-violation-detection-plugin)
    - [Task 4 — Installation](#task-4--installation)
    - [Task 5 — Add Linters](#task-5--add-linters)
    - [Task 6 — CI / CD (GitHub Actions)](#task-6--ci--cd-github-actions)
    - [Extra Task 7 — Add Sanitizers](#extra-task-7--add-sanitizers)
    - [Extra Task 8 — Introduce a Conan Package](#extra-task-8--introduce-a-conan-package)
  - [3. Evaluation Criteria](#3-evaluation-criteria)

---

### 1. Technical Solution Constraints

- **C++ standard**: C++20, extensions off.
- **Platforms**: Linux (GCC 11+ and Clang 14+) and Windows (MSVC 2022+). *macOS support is not required*.
- **Dependency manager**: Conan 2 (not Conan 1, not vcpkg).
- **Build system**: CMake 3.21.
- **Test framework**: Google Test (via Conan).
- **Third-party deps**: must be linked as a **shared** library.

### 2. Definition of done

> ⚠️ Send us back the link to your public repo at least 24 hrs prior tech interview. <br>
> **Send your link to the e-mails:** **evgenii.neruchek@agile-robots.com**, **niklas.hollstegge@agile-robots.com**

A final deliverable is a **public GitHub repository** containing:
- The history of your changes (incl. related CI pipelines).
- All source files (provided + your additions).
- `CMakeLists.txt` files, `conanfile.py`, `.clang-format`, `.clang-tidy`.
- `.github/workflows/` CI configuration.
- A `README.md` with build / usage instructions.
- A green CI run on the `main` branch demonstrating all checks pass.
- At least one tagged release (`v*`) with the install archives attached.


> ⚠️ **After the tech interview** please **make the repository private** forever, or remove it.

### 3. Technical interview

During the technical interview be ready to share your screen to make some changes in the project.

---

## Technical challenge

### 1. Provided Source Files

```
app/
└── src/main.cpp              

plugin/
├── include/plugin/plugin.h
└── src/plugin.cpp           

tests/
└── test_plugin.cpp         
```

---

### 2. Requirements

Complete every task below. Each section is evaluated independently.

Some tasks may have an _"Extra Goal"_, and some tasks are marked as _"Extra Task"_ (tasks 7 & 8). They're nice-to-have; finish it only if you have spare time.

> ⚠️ **Do not modify the provided source files unless a task explicitly requires it** (e.g., adding a new plugin). All other work should be in build-system files, configuration, and CI scripts.

> ⚠️ **We did this challenge too long on purpose**, please prioritize your work to get the maximum value possible within the time limit.

> ⚠️ **If you didn't fulfill all requirements/goals/tasks it's ok and expected**, it doesn't mean the challenge is failed. 
> Bring to the Technical interview everything you was able to finish in time.
> <br><br>_Example: Failing Windows or Linux pipeline is acceptable._

> ⚠️ You're **allowed to use any third-party help**, AI Agents etc. But you still **should be aware about every implementation detail**.

### Task 1 — Use CMake build system

Set up a **CMake** (3.21) build & testing for the project.

| Target | Kind | Notes                                                                                                                                    |
|---|---|------------------------------------------------------------------------------------------------------------------------------------------|
| `challenge` | Executable | Includes headers from `plugin/include/`. Does **not** link the plugin at build time — it loads it at runtime via `dlopen`/`LoadLibrary`. |
| `plugin` | Shared library | Links all of its dependencies as shared libraries. Ensures symbols are visible.                                                          |
| `plugin_tests` | Executable (test) | Loads the plugin dynamically (same as the main executable).                                                           |

### Task 2 — Add dependency management using Conan 2

Create a `conanfile.py` that:
- Requires all third-party dependencies. 
- Uses Conan generators and other modern approaches.

The definition of done is: project can be built **on both Windows (10/11) and Linux (Ubuntu 22.04)** via:
```bash
conan install . --build=missing -s build_type=Release
cmake --preset conan-release
cmake --build --preset conan-release
ctest --preset conan-release
```

### Task 3 — Add a SEGFAULT/Access violation detection plugin
 
Goal:
  * Create a new plugin that is able to detect a segfault, writes call stack to the text file. 
  * It should work on both Windows and Linux.
  * Write tests for it.

If this plugin requires some third-party dependencies, they should be installed via Conan as shared libraries (unless header-only).

You may need to adjust logic of plugins loading, in order to allow loading your plugin.

### Task 4 — Installation

Add CMake install rules so that `cmake --build --preset <preset> --target install` produces a **self-contained, relocatable** directory tree:

**Linux**
```
<prefix>/
├── bin/challenge
└── lib/
    ├── libplugin.so
    ├── libplugin_segfault.so          # your new plugin
    ├── third_party_dep_*.so.x.y.z
    └── ...
```

**Windows**
```
<prefix>/
└── bin/
    ├── challenge.exe
    ├── plugin.dll
    ├── plugin_segfault.dll             # your new plugin
    ├── third_party_dep_*.dll
    └── ...
    lib/
    ├── plugin.lib
    ├── plugin_segfault.dll             # your new plugin
    └── ...
```

All shared Conan dependencies (Boost, and any transitive shared libs) **must** be included in the installed tree.

After installation, the executable should start successfully and show all plugins are loaded:
```bash
> /.../<prefix>/bin/challenge
Loading plugin from: /.../<prefix>/.../libplugin.so
...
```

### Task 5 — Add Linters

**Base goal:** Configure the following static-analysis tools:
  
| Tool             | Scope |
|------------------|---|
| **clang-format** | All project C++ source and header files. Provide a config. |
| **clang-tidy**   | All project C++ source files. Provide a config. |


- Both must be runnable locally (e.g. via a script or some other tool).
- Both must run automatically in CI and **fail the build** on violations.

**Extra goal:** Linters should be runnable locally and in CI via `pre-commit`.

### Task 6 — CI / CD (GitHub Actions)

Create `.github/workflows/` pipeline(s) that run on `push` to `main`, on pull requests, **and** on `v*` tags.

The CI must include the following **jobs / stages**:

| Stage | Runs on | Description                                                                                                                        |
|---|---|------------------------------------------------------------------------------------------------------------------------------------|
| **Build & Test** | `ubuntu-latest` (GCC), `ubuntu-latest` (Clang), `windows-latest` (MSVC) | Conan install, CMake configure + build, `ctest`.                                                                                   |
| **Lint** | `ubuntu-latest` | Run clang-format (check mode) and clang-tidy. Fail on violations.                                                                  |
| **Install & Package** | same matrix as Build & Test | `cmake --install`, archive the result (`.tar.gz` on Linux, `.zip` on Windows). Upload as workflow artifacts.                       |
| **Release** | `ubuntu-latest` | **Only on `v*.*.*` tags.** Download all artifacts from previous stages and create a **GitHub Release** with the archives attached. |

### Extra Task 7 — Add Sanitizers

Add a CMake option (e.g. `-DENABLE_SANITIZERS=ON`) that enables **AddressSanitizer** for all project targets.

- Must work with GCC and Clang on Linux.
- Tests must pass under ASan without leaks or errors.
- CI must run the test suite with ASan enabled in the separate job (original Build&Test job is unchanged).

### Extra Task 8 — Introduce a Conan Package

Make the project itself consumable as a Conan 2 package.

1. The `conanfile.py` must support `conan create .` — building and packaging the library targets (plugins) and their headers.
2. Add a `test_package/` directory so that `conan test test_package/ challenge_project/<version>` validates the package.
3. The resulting Conan package cache (`.tgz`) should also be **uploaded as a release artifact alongside the install archives in Task 7**.

The definition of done is: conan package can be built and tested **on both Windows (10/11) and Linux (Ubuntu 22.04)** via:
```bash
conan create . --build=missing
```
Then, after downloading the conan package archive from release artifacts, this command should successfully install a package:
```bash
conan cache restore challenge-conan-package.tar.gz
```

---

### 3. Evaluation Criteria

| Area                  | What we look for |
|-----------------------|---|
| **Correctness**       | Everything builds and runs on both Linux and Windows. Tests pass. Installed tree is self-contained. |
| **CMake quality**     | Proper use of targets, generator expressions, install rules. No hard-coded paths. |
| **Conan integration** | Clean `conanfile.py`. `conan install`, `conan create`, and `conan test` all work. Shared-library option correctly propagated. |
| **CI robustness**     | Matrix covers all required platforms/compilers. Linters and sanitizers actually catch issues. Release uploads are complete. |
| **Code quality**      | Clean, idiomatic C++20. New plugin follows the same conventions as the existing one. |
| **Plugin robustness** |  |
| **Documentation**     | Brief but sufficient instructions in a top-level README for a new developer to clone, build, and run the project. |
