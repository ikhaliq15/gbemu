load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake", "configure_make")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "all_srcs",
    srcs = glob(["**"]),
)

cc_library(
    name = "include",
    hdrs = glob(["include/**"]),
    strip_include_prefix = "include",
)

COPTS = [
    "-fPIC",
    "-pthread",
    "-O3",
]

cmake(
    name = "SDL",
    cache_entries = {
        "CMAKE_C_FLAGS": "-fPIC",
        "CMAKE_OSX_ARCHITECTURES": "x86_64;arm64",
        "CMAKE_OSX_DEPLOYMENT_TARGET": "10.14",
        "CMAKE_POSITION_INDEPENDENT_CODE": "ON",
        "CMAKE_INSTALL_LIBDIR": "lib",
        "SDL_SHARED": "ON",
        "SDL_JOYSTICK": "ON",
        "SDL_HAPTIC": "ON",
        "SDL_COCOA": "ON",
        "SDL_METAL": "ON",
        "SDL_TEST": "OFF",
        "SDL_RENDER": "ON",
        "SDL_VIDEO": "ON",
    },
    copts = COPTS,
    lib_source = ":all_srcs",
    linkopts = select({
        "//conditions:default": [],
        "@bazel_tools//src/conditions:darwin": [
            "-Wl",
            "-framework",
            "CoreAudio,AudioToolbox",
        ],
    }),
    out_static_libs = select({
        "@bazel_tools//src/conditions:windows": [
            "libSDL2main.lib",
            "libSDL2.lib",
        ],
        "//conditions:default": [
            "libSDL2main.a",
            "libSDL2.a",
        ],
    }),
    targets = [
        "preinstall",
        "install",
    ],
)