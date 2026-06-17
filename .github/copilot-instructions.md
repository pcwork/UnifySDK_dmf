# Unify Host SDK - Copilot Instructions

## Project Overview

The **Unify Host SDK** is Silicon Labs' unified repository for Linux-based host applications that interface with IoT radios (Z-Wave, Zigbee, BLE, Matter, Thread). It provides protocol controllers that bridge IoT protocols to a unified MQTT-based architecture using the Unify Controller Language (UCL) and DotDot/Zigbee Cluster Library (ZCL).

**Main Applications:**
- **ZPC** (Z-Wave Protocol Controller) - Full certifiable Z-Wave controller
- **ZigPC** (Zigbee Protocol Controller) - Zigbee coordinator interface
- **AOXPC** (AoX Protocol Controller) - Bluetooth direction finding
- **GMS** (Group Management Service), Image Provider, UPVL, UPTI tools

## Tech Stack

- **Languages**: C/C++ (primary), Rust (growing), Python (scripting/tools)
- **Build System**: CMake 3.21+ with Ninja generator, cross-compilation via Docker
- **Testing**: Unity + CMock for unit tests (`BUILD_TESTING=ON`)
- **Code Quality**: clang-format 10, clang-tidy, .editorconfig
- **Dependencies**: MQTT (Mosquitto), SQLite (attribute store), Gecko SDK (for ZigPC), libboost, nlohmann-json
- **Target Platform**: Debian Bookworm (arm64/aarch64 for Raspberry Pi)
- **Rust**: Minimum version 1.64, integrated via CMakeCargo

## Directory Structure

```
applications/        # Protocol controllers and services (zpc, zigpc, aox, gms, etc.)
  └── <app>/
      ├── components/  # Application-specific components
      ├── CMakeLists.txt
      └── readme_*.md
components/          # Shared Unify Framework components (uic_* prefix required)
  ├── uic_attribute_store/  # Core attribute-based data model
  ├── uic_mqtt/             # MQTT client wrapper
  ├── uic_config/           # YAML configuration system
  ├── uic_dotdot_mqtt/      # DotDot/ZCL MQTT interface (auto-generated from ZAP)
  └── testframework/        # Unity/CMock testing infrastructure
cmake/               # Build toolchains and modules
  ├── arm64_debian.cmake    # Cross-compile toolchain for RPi
  └── include/              # Shared CMake utilities
doc/                 # Sphinx documentation (Markdown)
docker/              # Dockerfiles and build scripts for cross-compilation
```

## Build Instructions

### Quick Build (Docker-based)
```bash
# Build Docker image for arm64
./docker/build_docker.sh arm64 uic_arm64

# Start container and build
docker run -it --rm -v $PWD:$PWD -w $PWD uic_arm64
mkdir build && cd build
cmake -GNinja -DCMAKE_TOOLCHAIN_FILE=../cmake/arm64_debian.cmake ..
ninja               # Build binaries
ninja test          # Run unit tests
ninja deb           # Create Debian packages
```

### Local Build (Debian/Ubuntu)
```bash
# Install dependencies (see helper.mk or docker/Dockerfile)
make -f helper.mk setup  # Installs packages, Rust, Python deps

# Configure and build
cmake -B build -GNinja -DBUILD_TESTING=ON
ninja -C build
ctest --test-dir build
```

### Building Specific Applications
Disable unneeded apps to speed up builds:
```bash
cmake -GNinja -DBUILD_ZIGPC=OFF -DBUILD_AOXPC=OFF ..
```

### ZigPC Requirements
ZigPC requires **Gecko SDK** with EmberZNet library:
```bash
export GSDK_LOCATION=~/SimplicityStudio/SDKs/gecko_sdk/
docker run -it --rm --env GSDK_LOCATION -v $PWD:$PWD -w $PWD -v ${GSDK_LOCATION}:${GSDK_LOCATION} uic_arm64
```

## Coding Standards

### General Rules (see `doc/standards/coding-standard.md`)
- **Indentation**: 2 spaces (no tabs)
- **Line length**: 80 characters max
- **Filenames**: `lowercase_with_underscores` (except CMakeLists.txt, Dockerfile, Makefile)
- **Brace style**: K&R variant (function braces on new line, control structures inline)
- **Pointer alignment**: Right-aligned (`int *ptr`)
- **Comments**: Clear, concise; avoid redundant comments

### Naming Conventions
- **Components**: Prefix with `unify_` or application-specific (e.g., `zpc_`, `zigpc_`)
- **C files**: `component_name_functionality.c` (snake_case)
- **C++ classes**: `CapitalCase` for class names, `camelCase` for methods/fields
- **CMake variables**: `UPPERCASE_WITH_UNDERSCORES`
- **CMake functions**: lowercase with keyword args in `UPPERCASE`

### Code Formatting
- **clang-format**: Run before committing (uses `.clang-format` v10 config)
- **clang-tidy**: Enabled for portability/readability checks (see `.clang-tidy`)
- **Format command**: `clang-format -i <file>` (auto-formats in place)

### CMake Standards (see `doc/readme_cmake.md`)
- Declare components as SHARED libraries: `add_library(unify_my_component SHARED ...)`
- Use keyword arguments on separate lines (2-space indent)
- Mock generation: `target_add_mock(<target>)` creates `<target>_mock` library
- Unit tests: `target_add_unittest(<target> SOURCES test.c DEPENDS mock1 mock2)`

### Rust Integration
- Rust projects live alongside C/C++ code; built via `cargo_build()` in CMake
- Bindings to C libraries auto-generated; link to `UNIFY_BINARY_DIR` artifacts
- Set `UNIFY_BINARY_DIR` and `VERSION_STR` when using cargo standalone

## Git Workflow

### Branch Naming (see `CONTRIBUTING.md`)
- `feature/<description>` or `feature/GH-1234-description`
- `bugfix/<description>` or `bugfix/MC-5678-fix-crash`
- `refactor/`, `docs/`, `test/`, `experimental/`

### Commit Messages
- **Format**: `<summary line max 50 chars>`
- **Body**: Optional detailed description starting line 3
- **Trailers**: 
  - **REQUIRED**: `Signed-off-by: songhao wang <songhao.wang_1@signify.com>`
  - **If applicable**: `MC-xxxx` prefix in commit message if branch contains `MC-xxxx`
  - **Do NOT include**: `Co-authored-by` trailers

### Pull Requests
- **Title**: `Context: <message clearing explaining what commit does>`
- **Size**: < 300 lines (excluding auto-generated code); split larger changes
- **Testing**: Run unit tests locally before submitting (`ninja test`)
- **One issue per PR**: Don't combine multiple unrelated changes

## Common Patterns & Gotchas

### Unit Testing
- Tests reside in `<component>/test/` directories
- Use `target_add_unittest()` to auto-link mocks and dependencies
- Mock public headers only; use `target_add_mock(<target>)` to generate
- Run tests: `ctest --test-dir build` or `ninja test`

### Auto-generated Code (ZAP)
- DotDot/ZCL definitions in `components/uic_dotdot/` are generated from ZAP templates
- ZAP tool: `cmake/include/zap.cmake` orchestrates code generation
- Modify `.zapt` templates in `components/uic_dotdot/zap/`, not generated `.cpp` files

### Attribute Store Pattern
- Central data model for device state: hierarchical key-value store backed by SQLite
- Create/read/update via `attribute_store_*` API (see `components/uic_attribute_store/`)
- Attribute Resolver handles asynchronous read/write resolution
- Attribute Mapper translates between Unify attributes and DotDot/MQTT

### MQTT Integration
- All protocol controllers publish/subscribe via UCL on MQTT
- Topic structure: `ucl/by-unid/<UNID>/...` (Unify Node ID)
- DotDot MQTT layer auto-generates command/attribute handlers
- Test with: `mosquitto_sub -v -t 'ucl/#'`

### Cross-compilation Caveats
- **Docker is primary build method** for release builds
- ARM binaries can run in Docker via QEMU binfmt (enable with `update-binfmts`)
- S2 crypto library (Z-Wave security) is pre-built; lives in `applications/zpc/components/zwave/zwave_transports/s2/libs/zw-libs2/`

### Known Workarounds (TODO/FIXME/HACK patterns found)
- **TODO**: Many in transport_service, s2_inclusion, and various test files (grep results show 100+ instances)
- **HACK**: Found in `helper.mk`, zigbee_host, and attribute resolvers
- **Workarounds**: Test harnesses often include `workaround.hpp` headers for mocking edge cases

## Essential Files & Resources

- **README.md**: High-level intro, links to docs site
- **CONTRIBUTING.md**: Git workflow, PR guidelines, commit format
- **doc/readme_building.md**: Detailed build instructions
- **doc/readme_developer.md**: Developer guide, directory structure
- **doc/standards/coding-standard.md**: Full coding standard
- **doc/readme_cmake.md**: CMake conventions and helper functions
- **doc/readme_rust.md**: Rust integration guide
- **helper.mk**: Makefile for quick setup and builds (Debian)
- **Dockerfile**: Docker build environment definition

## Quick Reference

**Build just ZPC**:
```bash
make -f helper.mk zpc/default  # configure, build, test
```

**Format code**:
```bash
clang-format -i applications/zpc/components/*/src/*.c
```

**Generate documentation**:
```bash
ninja -C build doxygen  # Generates Doxygen docs in build/doxygen/
```

**Troubleshooting**:
- Git LFS not pulled? `git lfs pull`
- Rust version too old? `rustup update` (need >= 1.64)
- CMake fails? Check `build/CMakeFiles/CMakeOutput.log`
- Unit test failures? Check test logs in `build/Testing/Temporary/`

---

**For questions or support**, refer to `doc/getting_started_as_developer.md` or internal Signify documentation.
