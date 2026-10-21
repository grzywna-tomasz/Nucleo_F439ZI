#!/usr/bin/env node

const path = require("node:path");
const {
  collectBrowsePaths,
  findCompileDatabases,
  readCompileDatabase,
  resolveConfiguredPath
} = require("./sync");
const { updateSettingsFile } = require("./settings");

async function main() {
  const workspaceFolder = process.cwd();
  const settingsPath = path.join(workspaceFolder, ".vscode", "settings.json");
  const buildDirectory = resolveConfiguredPath(
    process.env.CMAKE_INTELLISENSE_BUILD_DIRECTORY || "${workspaceFolder}/build",
    workspaceFolder
  );
  const pinnedDatabase = process.env.CMAKE_INTELLISENSE_COMPILE_COMMANDS || "";
  const databases = await findCompileDatabases(buildDirectory);
  let databasePath;

  if (pinnedDatabase) {
    databasePath = resolveConfiguredPath(pinnedDatabase, workspaceFolder);
    if (!databases.some((database) => database.path === databasePath)) {
      throw new Error(`Pinned compile database was not found under ${buildDirectory}: ${databasePath}`);
    }
  } else {
    databasePath = databases[0]?.path;
  }

  if (!databasePath) {
    throw new Error(`No compile_commands.json found under ${buildDirectory}. Configure CMake first.`);
  }

  const commands = readCompileDatabase(databasePath);
  const browsePaths = collectBrowsePaths(commands, workspaceFolder);
  const changed = updateSettingsFile(settingsPath, {
    "C_Cpp.default.compileCommands": databaseSettingPath(databasePath, workspaceFolder),
    "C_Cpp.default.browse.path": browsePaths
  });

  console.log(`${changed ? "Updated" : "Already up to date"} ${settingsPath}`);
  console.log(`Compile database: ${databasePath} (${commands.length} commands)`);
  console.log(`Browse paths: ${browsePaths.length}`);
}

function databaseSettingPath(databasePath, workspaceFolder) {
  const relativePath = path.relative(workspaceFolder, databasePath);
  if (relativePath === "" || (!relativePath.startsWith(`..${path.sep}`) && relativePath !== ".." && !path.isAbsolute(relativePath))) {
    return relativePath === ""
      ? "${workspaceFolder}/compile_commands.json"
      : `\${workspaceFolder}/${relativePath.split(path.sep).join("/")}`;
  }
  return databasePath;
}

main().catch((error) => {
  console.error(`CMake IntelliSense settings update failed: ${error instanceof Error ? error.message : String(error)}`);
  process.exitCode = 1;
});
