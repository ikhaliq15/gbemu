"""
Internal dependencies declaration

After adding a new dependency, run `bazel run @hedron_compile_commands//:refresh_all` for code completion to work.
"""

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def internal_deps():
    git_repository(
        name = "rules_foreign_cc",
        remote = "https://github.com/bazelbuild/rules_foreign_cc",
        commit = "d33d862abb4ce3ba178547ef58c9fcb24cec38ca",
        shallow_since = "1677931962 +0000",
    )

    git_repository(
        name = "gtest",
        remote = "https://github.com/google/googletest",
        branch = "v1.14.x",
    )

    http_archive(
        name = "com_grail_bazel_compdb",
        strip_prefix = "bazel-compilation-database-0.5.2",
        urls = ["https://github.com/grailbio/bazel-compilation-database/archive/0.5.2.tar.gz"],
    )

    http_archive(
        name = "com_github_libsdl_sdl2",
        build_file = "//rules:sdl2.BUILD",
        sha256 = "e2ac043bd2b67be328f875043617b904a0bb7d277ba239fe8ac6b9c94b85cbac",
        strip_prefix = "SDL-dca3fd8307c2c9ebda8d8ea623bbbf19649f5e22",
        urls = ["https://github.com/libsdl-org/SDL/archive/dca3fd8307c2c9ebda8d8ea623bbbf19649f5e22.zip"],
    )

    git_repository(
        name = "nativefiledialog_git",
        build_file = "//rules:nfd.BUILD",
        commit = "5786fabceeaee4d892f3c7a16b243796244cdddc",
        remote = "https://github.com/btzy/nativefiledialog-extended.git",
    )