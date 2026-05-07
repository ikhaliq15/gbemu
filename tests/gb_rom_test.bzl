"""Bazel macro for declaring GBEmu ROM-based integration tests.

Each invocation produces a single `py_test` that runs `tests/gb_rom_test.py`
against one ROM. Declaring one target per ROM lets Bazel parallelise, cache,
and report failures per ROM without any test-runner gymnastics in Python.
"""

load("@rules_python//python:defs.bzl", "py_test")

_GBEMU_BINARY = "//gbemu:gbemu_exe"
_HARNESS_SRC = "//tests:gb_rom_test.py"

_VALID_MODES = ("serial_ascii", "serial_binary", "image")

def gb_rom_test(
        name,
        rom,
        mode = None,
        reference_image = None,
        rom_timeout_seconds = 120,
        pass_text = None,
        pass_bytes = None,
        size = "medium",
        tags = None):
    """Declare a GBEmu ROM integration test.

    Args:
      name: Bazel target name.
      rom: Label of the .gb ROM file to run.
      mode: One of:
        - "serial_ascii": look for an ASCII serial marker such as "Passed"
        - "serial_binary": look for a binary serial byte sequence such as
          [3, 5, 8, 13, 21, 34]
        - "image": compare the dumped framebuffer against a reference image
      reference_image: Required when mode="image"; label of the golden PNG.
      rom_timeout_seconds: Wall-clock budget for the emulator to run.
      pass_text: Required when mode="serial_ascii"; ASCII success marker.
      pass_bytes: Required when mode="serial_binary"; list of byte values
        in the range 0..255.
      size: Bazel test size.
      tags: Extra Bazel tags.
    """
    if mode == None:
        fail("gb_rom_test: mode is required (one of {})".format(_VALID_MODES))
    if mode not in _VALID_MODES:
        fail("gb_rom_test: mode must be one of {}, got {}".format(_VALID_MODES, mode))

    if mode != "image" and reference_image != None:
        fail("gb_rom_test: reference_image is only valid when mode='image'")
    if mode != "serial_ascii" and pass_text != None:
        fail("gb_rom_test: pass_text is only valid when mode='serial_ascii'")
    if mode != "serial_binary" and pass_bytes != None:
        fail("gb_rom_test: pass_bytes is only valid when mode='serial_binary'")

    if mode == "image":
        if reference_image == None:
            fail("gb_rom_test: reference_image is required when mode='image'")
    elif mode == "serial_ascii":
        if pass_text == None:
            fail("gb_rom_test: pass_text is required when mode='serial_ascii'")
    elif mode == "serial_binary":
        if pass_bytes == None:
            fail("gb_rom_test: pass_bytes is required when mode='serial_binary'")
        if type(pass_bytes) != "list":
            fail("gb_rom_test: pass_bytes must be a list of integers")
        if len(pass_bytes) == 0:
            fail("gb_rom_test: pass_bytes must not be empty")
        for byte in pass_bytes:
            if type(byte) != "int":
                fail("gb_rom_test: pass_bytes must contain only integers")
            if byte < 0 or byte > 255:
                fail("gb_rom_test: pass_bytes values must be in the range 0..255")

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
    ]
    data = [_GBEMU_BINARY, ":" + rom_alias]
    deps = [
        "@rules_python//python/runfiles",
    ]

    if mode == "serial_ascii":
        args.append("--pass-text=" + pass_text)
    elif mode == "serial_binary":
        args.append("--pass-bytes=" + ",".join([str(byte) for byte in pass_bytes]))

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
