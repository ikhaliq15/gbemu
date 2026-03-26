py_test(
    name = "blargg_cpu_instrs",
    srcs = ["blargg_cpu_instrs_harness.py"],
    data = [
        "//gbemu:gbemu-bin",
        "//gbemu/data:opcodes",
        "@blargg_test_roms//:cpu_instrs/individual",
    ],
    main = "blargg_cpu_instrs_harness.py",
    deps = [
        "//gbemu:gbemu-bin",
    ],
)
