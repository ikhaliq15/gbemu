load("@rules_python//python:defs.bzl", "py_test")

def blargg_test(name, roms):
    py_test(
        name = name,
        srcs = ["//tests/blargg:blargg_harness.py"],
        data = [
            "//gbemu:gbemu_exe",
        ] + roms,
        main = "blargg_harness.py",
        deps = [
            "//gbemu:gbemu_exe",
            "@rules_python//python/runfiles",
        ],
    )
