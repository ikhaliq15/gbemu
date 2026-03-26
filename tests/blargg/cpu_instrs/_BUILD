py_test(
    name = "blargg_cpu_instrs",
    srcs = ["blargg_cpu_instrs_harness.py"],
    data = [
        "//gbemu:gbemu-bin",
        "//gbemu/data:opcodes",
        "//roms/blargg/cpu_instrs/individual:cpu_instrs_individual",
    ],
    main = "blargg_cpu_instrs_harness.py",
    deps = ["//gbemu:gbemu-bin"],
)
