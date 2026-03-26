py_test(
    name = "dmg_acid2",
    srcs = ["dmg_acid2_harness.py"],
    data = [
        "//gbemu:gbemu-bin",
        "@dmg_acid2//:img/reference-dmg.png",
        "@dmg_acid2_rom//file",
    ],
    main = "dmg_acid2_harness.py",
    deps = [
        "//gbemu:gbemu-bin",
        "@gbemu_pip//pillow",
        "@rules_python//python/runfiles",
    ],
)
