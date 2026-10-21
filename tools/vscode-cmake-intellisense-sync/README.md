# CMake IntelliSense Sync

A local VS Code extension that keeps the Microsoft C/C++ extension aligned with
the CMake compilation database for this workspace.

## Run locally

1. Open the repository root in VS Code.
2. Select **Run CMake IntelliSense Sync Extension** in the Run and Debug view.
3. Press F5 to launch an Extension Development Host with this workspace loaded.

Alternatively, set the extension folder as the `--extensionDevelopmentPath`
when launching an Extension Development Host. For use outside the development
host, package and install the extension using VS Code's extension packaging
tools.

## Trigger from a VS Code task

Run **Tasks: Run Task** from the Command Palette and select **Update C/C++
IntelliSense settings**. This is a standalone process task and does not require
the extension to be installed or activated in the current window. It uses the
newest compilation database and reports success or failure in the task
terminal. Set `CMAKE_INTELLISENSE_BUILD_DIRECTORY` or
`CMAKE_INTELLISENSE_COMPILE_COMMANDS` in the task environment to override
automatic database selection.

## Behavior

- Searches `${workspaceFolder}/build` recursively for `compile_commands.json`.
- Uses the most recently updated database unless one is explicitly pinned.
- Updates only `C_Cpp.default.compileCommands` and `C_Cpp.default.browse.path`
  in workspace settings; other settings and JSONC comments are preserved.
- Watches compilation databases for CMake reconfiguration and refreshes
  IntelliSense automatically.
- Derives browse paths from the compiled source directories and the `-I`,
  `-isystem`, `-iquote`, and `-idirafter` compiler arguments. Directories from
  unrelated vendored examples are therefore not added to the browse database.

Use **CMake IntelliSense: Select Compile Database** to pin a particular build
directory, or clear `cmakeIntelliSenseSync.compileCommands` in workspace
settings to return to automatic selection. Use **CMake IntelliSense: Sync From
Build Directory** to synchronize immediately.

The project must enable `CMAKE_EXPORT_COMPILE_COMMANDS` and be configured at
least once. The checked-in root `CMakeLists.txt` already enables it.

## Test

From this directory, run:

```sh
npm test
```
