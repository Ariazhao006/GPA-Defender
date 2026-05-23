# GPA Defender Project Initialization Design

## Summary

This document defines the engineering initialization for the existing GPA Defender C++ project. The chosen direction is a layered reorganization focused on developer readiness: adopt CMake as the primary build entrypoint, reorganize the repository into standard source/include/test/docs/script directories, and preserve current gameplay behavior while improving build, test, and IDE workflows.

## Goals

- Make CMake the canonical build and test entrypoint.
- Reorganize the repository into a conventional development-friendly layout.
- Separate core game logic from executable entrypoints.
- Keep existing gameplay behavior unchanged during initialization.
- Preserve lightweight local test workflows without adding external dependencies.
- Update documentation and editor tasks to reflect the new structure.

## Non-Goals

- No gameplay feature work or balance changes.
- No large-scale refactor of class responsibilities beyond what is required by file moves and target separation.
- No packaging, installer, or release pipeline setup in this phase.
- No third-party test framework or package manager integration in this phase.
- No CI configuration in this phase.

## Current State

The repository already contains a working C++ tower-defense backend, build helper scripts, VS Code task files, smoke-test style executables, and multiple design documents. However, the workspace is flat, build outputs are mixed into the project root, documentation references are partially stale, and the main build flow depends on hand-written compiler command lines.

## Chosen Approach

The project will use a layered reorganization approach:

1. Keep the implementation scope narrow and behavior-preserving.
2. Move the repository to a final-form directory structure now, rather than simulating it through transitional wrappers.
3. Introduce a single CMake-based build graph for the main application and smoke tests.
4. Retain helper scripts and VS Code support, but make them thin wrappers over CMake.

This approach was chosen over a full one-shot architectural rewrite because it gives the repository a clean long-term shape without mixing initialization with unrelated logic changes.

## Target Repository Layout

```text
GPA defender/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── cmake/
├── include/
│   └── gpa_defender/
├── src/
├── tests/
├── scripts/
├── docs/
│   ├── superpowers/
│   │   └── specs/
│   └── ...
└── assets/
```

### Directory Roles

- `include/gpa_defender/`: project headers exposed to app and test targets.
- `src/`: core implementation files and the application entrypoint.
- `tests/`: smoke-test executables and future test sources.
- `scripts/`: helper shell/batch scripts that delegate to CMake.
- `docs/`: backend notes, design documents, and future engineering docs.
- `assets/`: binary and reference assets such as proposal PDFs and future media.
- `cmake/`: reserved for helper modules if the top-level CMake file needs to stay clean.

## Build System Design

### Language and Tooling Baseline

- C++ standard: C++17
- No third-party dependencies
- Primary generators: platform default, with VS Code compatibility preserved
- Build tree: `build/` in the repository root

### CMake Targets

#### `gpa_defender_core`

A static library containing the gameplay and engine logic shared by the application and tests. This target is expected to include modules such as:

- `Enemy`
- `DefenseTower`
- `Obstacle`
- `Questionnaire`
- `PlayerStats`
- `Block`
- `GameEngine`
- `WaveManager`
- `GameUI` if required by test targets

The exact target membership should reflect current compile dependencies, but the intent is to centralize reusable game logic into one library.

#### `gpa_defender_app`

The main executable built from `src/main.cpp`, linked against `gpa_defender_core`. This becomes the canonical playable/backend demonstration target.

#### Smoke Test Executables

At minimum, the initialization should provide CMake targets for the existing smoke-test style flows, such as:

- `backend_smoke`
- `full_flow_smoke`

These targets should be registered with CTest so the standard validation flow is:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

### Explicit Constraints

- No shared-library split in this phase.
- No install rules required.
- No package/export config required.
- No external testing framework required.

## Migration Rules

### File Placement

- Move implementation `.cpp` files into `src/`.
- Move public/project headers into `include/gpa_defender/`.
- Move the current `main.cpp` into `src/main.cpp`.
- Move smoke-test sources such as `BackendLoopSmokeTest.cpp` into `tests/`.
- Move build helper scripts into `scripts/`.
- Move engineering and gameplay documentation into `docs/`.
- Move proposal and other binary/reference assets into `assets/`.

### Include Strategy

Header includes should be normalized to the new include root so the project can compile through explicit include directories rather than relying on all files being in the repository root.

The preferred steady-state pattern is:

```cpp
#include "gpa_defender/GameEngine.h"
```

If the existing codebase requires an intermediate compatibility pass, that pass should be kept as small as possible and removed within the same initialization change.

### Build Artifact Policy

- Build outputs belong under `build/`.
- Checked-in `.exe` artifacts should be removed from the root during the initialization change.
- `.gitignore` should exclude `build/`, executables, debug symbols where appropriate, and editor/system noise.

## Editor and Script Integration

### VS Code

Existing `.vscode/tasks.json` and `launch.json` should be updated to align with the CMake-based layout. The new default experience should support:

- configuring/building the project through the generated build tree
- launching or debugging the main executable from the build output

The implementation may either keep generic shell tasks or move toward CMake-friendly tasks, but the final behavior must match the new repository structure.

### Scripts

Existing helper scripts should be preserved in `scripts/`, but simplified so they call CMake rather than embedding long compiler command lines. Their purpose becomes convenience, not build-system authority.

## Documentation Changes

### Root README

The repository should gain or update a root `README.md` that clearly covers:

- project purpose
- repository structure
- prerequisites
- build instructions using CMake
- test instructions using CTest
- development notes for VS Code users

### Existing Docs

Existing documentation should be moved under `docs/` and updated when path moves or stale file references would otherwise break navigation. This includes fixing references to missing files or obsolete manual compile steps.

## Error Handling and Risk Management

The highest risks in this initialization are path breakage, include breakage, and silent divergence between application and smoke-test compile inputs. To control that risk:

- keep file moves mechanical
- avoid mixing behavior changes with path changes
- validate the main executable and smoke tests through CMake before considering the work complete
- keep scripts and editor configuration aligned with the same build tree

## Validation Criteria

The initialization is complete only if all of the following are true:

1. The repository root contains only top-level project entry files and a small set of meta files, not scattered source files or built executables.
2. `cmake -S . -B build` succeeds.
3. `cmake --build build` succeeds.
4. `ctest --test-dir build --output-on-failure` succeeds for the registered smoke tests.
5. The main application target builds from the new structure.
6. VS Code build/debug configuration matches the reorganized layout.
7. `README.md` accurately describes the new structure and commands.
8. `.gitignore` prevents common generated artifacts from polluting the root.

## Implementation Boundary for the Next Phase

The next phase should produce an implementation plan that covers:

- directory creation and file relocation order
- target definitions and source grouping in CMake
- include-path updates
- script and VS Code config updates
- documentation migration
- verification steps

That implementation plan should stay focused on initialization and must not expand into unrelated refactoring or gameplay work.
