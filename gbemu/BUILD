load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "gbemu-bin",
    srcs = glob([
      "*.cpp",
      "*.h",
      ]),
  visibility = ["//visibility:public"],
  linkopts = select({
      "//conditions:default": [],
      "@bazel_tools//src/conditions:darwin": [
          "-framework",
          "AudioToolbox",
          "-framework",
          "IOKit",
          "-framework",
          "Carbon",
          "-framework",
          "Cocoa",
          "-framework",
          "GameController",
          "-framework",
          "ForceFeedback",
          "-framework",
          "CoreHaptics",
          "-framework",
          "Metal",
          "-framework",
          "CoreVideo",
          "-liconv"
      ],
  }),
  deps = [
    "@json//:json",
    "@com_github_libsdl_sdl2//:SDL",
    "@com_github_libsdl_sdl2//:include",
    ]
)

cc_test(
  name = "gbemu_test",
  size="small",
  srcs = [
    "test/bitutils_test.cpp",
    "test/cpu_test.cpp",
    "alu.cpp",
    "alu.h",
    "bitutils.h",
    "cpu.h",
    "cpu.cpp",
    "opcode.h",
    "opcode.cpp",
    "operand.h",
    "ram.h",
    "ram.cpp",
    ],
  data = [
    "data/opcodes.json",
    ],
  deps = [
    "@gtest//:gtest_main",
    "@json//:json",
    "@com_github_libsdl_sdl2//:SDL",
    "@com_github_libsdl_sdl2//:include",
    ]
)