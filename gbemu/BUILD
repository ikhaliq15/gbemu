load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//gbemu:__subpackages__"])

cc_library(
    name = "gbemu-bitutils",
    srcs = [],
    hdrs = [
        "bitutils.h",
    ],
)

cc_library(
    name = "gbemu-joypad",
    srcs = [
        "joypad.cpp",
    ],
    hdrs = [
        "joypad.h",
    ],
    deps = [
        ":gbemu-bitutils",
        ":gbemu-ram",
        "@sdl2//:SDL2",
    ],
)

cc_library(
    name = "gbemu-ram",
    srcs = [
        "cartridge.cpp",
        "ram.cpp",
    ],
    hdrs = [
        "cartridge.h",
        "ram.h",
    ],
    deps = [
        ":gbemu-bitutils",
    ],
)

cc_library(
    name = "gbemu-lib",
    srcs = [
        "alu.cpp",
        "alu.h",
        "config.cpp",
        "cpu.cpp",
        "gameboy.cpp",
        "opcode.cpp",
        "ppu.cpp",
        "timer.cpp",
    ],
    hdrs = [
        "alu.h",
        "config.h",
        "cpu.h",
        "gameboy.h",
        "opcode.h",
        "operand.h",
        "ppu.h",
        "timer.h",
    ],
    deps = [
        ":gbemu-bitutils",
        ":gbemu-joypad",
        ":gbemu-ram",
        "@json",
        "@sdl2//:SDL2",
    ],
)

cc_binary(
    name = "gbemu-bin",
    srcs = [
        "main.cpp",
    ],
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
            "-liconv",
            "-framework",
            "UniformTypeIdentifiers",
            "-framework",
            "AppKit",
        ],
    }),
    visibility = ["//visibility:public"],
    deps = [
        ":gbemu-lib",
        "@argparse",
    ],
)
