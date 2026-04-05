# Development Guide

This document covers the day-to-day development workflow for GBEmu. The public project overview lives in [README.md](README.md).

## Prerequisites

- Bazel 8.x
- Python 3.11
- `pre-commit`
- On macOS, Xcode command line tools
- On Linux, GTK 3 development headers if you are building the desktop UI

## Common Commands

Build the main executable:

```bash
bazel build //gbemu:gbemu_exe
```

Run the desktop UI with a local ROM:

```bash
bazel run //gbemu:gbemu_exe -- --rom_file /absolute/path/to/game.gb
```

Run in headless mode:

```bash
bazel run //gbemu:gbemu_exe -- --headless --rom_file /absolute/path/to/test.gb
```

Build the macOS app bundle:

```bash
bazel build //dist/macos:GBEmu
```

Run the test suite used in CI on Linux:

```bash
bazel test --skip_incompatible_explicit_targets -- //... -//dist/...
```

## Local Tooling

Install the repository hooks once:

```bash
python3 -m pip install pre-commit
pre-commit install
```

Run the hooks manually across the repository:

```bash
pre-commit run --all-files
```

## Editor Integration

Generate `compile_commands.json` for `clangd` after a successful build:

```bash
bazel run @hedron_compile_commands//:refresh_all
```

This creates `compile_commands.json` at the repository root for editors such as VS Code, Neovim, and CLion.

For VS Code, prefer the `clangd` extension and disable the Microsoft C/C++ IntelliSense engine to avoid duplicate indexing.

```json
{
  "C_Cpp.intelliSenseEngine": "disabled"
}
```

## Tests and ROM Data

The repository uses a mix of unit tests and ROM-based emulator checks. The ROMs used in automated tests are fetched through Bazel module dependencies rather than committed directly to the repository.

Local `roms/` directories are useful for manual debugging, but they should stay untracked. Do not commit commercial game ROMs, BIOS dumps, or other redistributable game assets.

## Release Workflow

Tagged releases are built by GitHub Actions. Pushing a tag like `v0.0.1` triggers the macOS release workflow, which:

- derives the release version from the tag
- stamps the Bazel build with that version
- uploads a versioned `GBEmu` macOS zip to GitHub Releases

The main CI workflow runs on pull requests and pushes to `main`. Linux builds exclude the macOS packaging targets, and the macOS bundle is built in a separate job.

## Repository Hygiene

- Keep user-facing docs in [README.md](README.md).
- Keep contributor and workflow details in this file.
- Avoid committing local editor state, generated Bazel outputs, app bundles, or personal ROM collections.
