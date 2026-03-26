# Developer Documentation

## Generate compile_commands.json for clangd/auto-complete

To get auto-complete and other intellisense features to work in IDEs and VSCode, we generate the standard `compile_commands.json` file tools expect.

First, build the project with

```console
$ bazel build //...
```

Then, generate the `compile_commands.json` file with

```console
$ bazel run @hedron_compile_commands//:refresh_all
```

There should now be a `compile_commands.json` in the root directory of the project.

### VSCode

To use this with VSCode, install the clangd extension: https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd

If you have the C/C++ VSCode extensino from microsoft, it is recommended to disable it's intellisense in your `settings.json` to avoid issues between the two tools.

```json
{
    ...
    "C_Cpp.intelliSenseEngine": "disabled",
    ...
}
```