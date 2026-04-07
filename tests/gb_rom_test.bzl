"""Bazel macro for declaring GBEmu ROM-based integration tests.

Each invocation produces a single `py_test` that runs `tests/gb_rom_test.py`
against one ROM. Declaring one target per ROM lets Bazel parallelise, cache,
and report failures per ROM without any test-runner gymnastics in Python.
"""

load("@rules_python//python:defs.bzl", "py_test")

_GBEMU_BINARY = "//gbemu:gbemu_exe"
_HARNESS_SRC = "//tests:gb_rom_test.py"

def gb_rom_test(
        name,
        rom,
        mode = "serial",
        reference_image = None,
        rom_timeout_seconds = 120,
        pass_marker = "Passed",
        size = "medium",
        tags = None):
    """Declare a GBEmu ROM integration test.

    Args:
      name: Bazel target name.
      rom: Label of the .gb ROM file to run.
      mode: "serial" (Blargg-style serial marker) or "image" (framebuffer
        compare).
      reference_image: Required when mode="image"; label of the golden PNG.
      rom_timeout_seconds: Wall-clock budget for the emulator to run.
      pass_marker: Serial-mode success string.
      size: Bazel test size.
      tags: Extra Bazel tags.
    """
    if mode not in ("serial", "image"):
        fail("gb_rom_test: mode must be 'serial' or 'image', got " + mode)
    if mode == "image" and reference_image == None:
        fail("gb_rom_test: reference_image is required when mode='image'")

    # Wrap the ROM (and reference image) in local aliases with paren-free
    # names. Some upstream ROM filenames — and therefore some test target
    # names — contain `(` and `)`, which break Bazel's `$(rlocationpath ...)`
    # parser (it matches parentheses greedily). The public `name` may keep
    # them; only the label fed to `$(rlocationpath ...)` must be sanitized.
    sanitized = name.replace("(", "").replace(")", "")
    rom_alias = sanitized + "_rom"
    native.alias(name = rom_alias, actual = rom, visibility = ["//visibility:private"])

    args = [
        "--gbemu=$(rlocationpath {})".format(_GBEMU_BINARY),
        "--rom=$(rlocationpath :{})".format(rom_alias),
        "--mode=" + mode,
        "--rom-timeout={}".format(rom_timeout_seconds),
        "--pass-marker=" + pass_marker,
    ]
    data = [_GBEMU_BINARY, ":" + rom_alias]
    deps = [
        "@rules_python//python/runfiles",
    ]

    if mode == "image":
        ref_alias = sanitized + "_reference"
        native.alias(
            name = ref_alias,
            actual = reference_image,
            visibility = ["//visibility:private"],
        )
        args.append("--reference-image=$(rlocationpath :{})".format(ref_alias))
        data.append(":" + ref_alias)
        deps.append("@gbemu_pip//pillow")

    py_test(
        name = name,
        srcs = [_HARNESS_SRC],
        main = "gb_rom_test.py",
        args = args,
        data = data,
        deps = deps,
        size = size,
        tags = tags or [],
    )
