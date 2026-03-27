load("@rules_python//python:defs.bzl", "py_test")

def cpu_instrs_test(name, roms):
    py_test(
        name = name,
        srcs = ["blargg_cpu_instrs_harness.py"],
        data = [
            "//gbemu:gbemu_exe",
        ] + roms,
        main = "blargg_cpu_instrs_harness.py",
        deps = [
            "//gbemu:gbemu_exe",
            "@rules_python//python/runfiles",
        ],
    )
