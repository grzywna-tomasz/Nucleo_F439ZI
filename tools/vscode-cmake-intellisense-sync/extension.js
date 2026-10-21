const fs = require("node:fs");
const path = require("node:path");
const vscode = require("vscode");
const {
  collectBrowsePaths,
  findCompileDatabases,
  readCompileDatabase,
  resolveConfiguredPath
} = require("./sync");

const BUILD_DIRECTORY_SETTING = "cmakeIntelliSenseSync.buildDirectory";
const DATABASE_SETTING = "cmakeIntelliSenseSync.compileCommands";

function activate(context) {
  const output = vscode.window.createOutputChannel("CMake IntelliSense Sync");
  context.subscriptions.push(output);

  const folder = vscode.workspace.workspaceFolders?.[0];
  if (!folder) {
    output.appendLine("No workspace folder is open; synchronization is inactive.");
    return;
  }

  const configuration = vscode.workspace.getConfiguration(undefined, folder.uri);
  const resolveBuildDirectory = () => resolveConfiguredPath(
    configuration.get(BUILD_DIRECTORY_SETTING, "${workspaceFolder}/build"),
    folder.uri.fsPath
  );
  let activeDatabasePath = null;
  let refreshTimer = null;
  let refreshInProgress = false;

  async function syncDatabase(databasePath) {
    const entries = readCompileDatabase(databasePath);
    const browsePaths = collectBrowsePaths(entries, folder.uri.fsPath);
    const databaseSettingPath = resolveConfiguredPathForSetting(databasePath, folder.uri.fsPath);
    const cppConfiguration = vscode.workspace.getConfiguration("C_Cpp.default", folder.uri);

    await Promise.all([
      cppConfiguration.update("compileCommands", databaseSettingPath, vscode.ConfigurationTarget.Workspace),
      cppConfiguration.update("browse.path", browsePaths, vscode.ConfigurationTarget.Workspace)
    ]);
    activeDatabasePath = databasePath;
    output.appendLine(`Using ${databasePath} (${entries.length} compile commands, ${browsePaths.length} browse paths).`);
    vscode.window.setStatusBarMessage("CMake IntelliSense settings synchronized", 5000);
  }

  async function syncLatestDatabase() {
    const buildDirectory = resolveBuildDirectory();
    const databases = await findCompileDatabases(buildDirectory);
    const pinnedDatabase = configuration.get(DATABASE_SETTING, "");
    let selected;
    if (pinnedDatabase) {
      selected = resolveConfiguredPath(pinnedDatabase, folder.uri.fsPath);
      if (!databases.some((candidate) => candidate.path === selected)) {
        throw new Error(`Pinned compile database was not found under ${buildDirectory}: ${selected}`);
      }
    } else {
      selected = databases[0]?.path;
    }

    if (!selected) {
      throw new Error(`No compile_commands.json found under ${buildDirectory}. Configure the CMake project first.`);
    }
    await syncDatabase(selected);
    return selected;
  }

  async function refresh(force = false) {
    if (refreshInProgress) {
      scheduleRefresh();
      return;
    }
    refreshInProgress = true;
    try {
      if (force) {
        await syncLatestDatabase();
      } else {
        const databases = await findCompileDatabases(resolveBuildDirectory());
        const pinnedDatabase = configuration.get(DATABASE_SETTING, "");
        const selected = pinnedDatabase
          ? resolveConfiguredPath(pinnedDatabase, folder.uri.fsPath)
          : databases[0]?.path;
        if (!selected) {
          output.appendLine("No compile_commands.json found. Configure the CMake project first.");
        } else if (!databases.some((candidate) => candidate.path === selected)) {
          throw new Error(`Pinned compile database was not found: ${selected}`);
        } else if (selected !== activeDatabasePath || !fs.existsSync(selected)) {
          await syncDatabase(selected);
        }
      }
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      output.appendLine(`Synchronization failed: ${message}`);
      void vscode.window.showErrorMessage(`CMake IntelliSense Sync: ${message}`);
    } finally {
      refreshInProgress = false;
    }
  }

  function scheduleRefresh() {
    if (refreshTimer) {
      clearTimeout(refreshTimer);
    }
    refreshTimer = setTimeout(() => {
      refreshTimer = null;
      void refresh();
    }, 500);
  }

  context.subscriptions.push(
    vscode.commands.registerCommand("cmakeIntelliSenseSync.syncNow", () => refresh(true)),
    vscode.commands.registerCommand("cmakeIntelliSenseSync.selectCompileDatabase", async () => {
      try {
        const databases = await findCompileDatabases(resolveBuildDirectory());
        if (databases.length === 0) {
          void vscode.window.showWarningMessage("No compile_commands.json files were found. Configure CMake first.");
          return;
        }
        const selected = await vscode.window.showQuickPick(
          databases.map((database) => ({
            label: path.relative(folder.uri.fsPath, database.path).split(path.sep).join("/"),
            description: new Date(database.mtimeMs).toLocaleString(),
            path: database.path
          })),
          { placeHolder: "Select the CMake build directory to use for IntelliSense" }
        );
        if (!selected) {
          return;
        }
        await configuration.update(
          DATABASE_SETTING,
          `\${workspaceFolder}/${path.relative(folder.uri.fsPath, selected.path).split(path.sep).join("/")}`,
          vscode.ConfigurationTarget.Workspace
        );
        await syncDatabase(selected.path);
      } catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        output.appendLine(`Database selection failed: ${message}`);
        void vscode.window.showErrorMessage(`CMake IntelliSense Sync: ${message}`);
      }
    }),
    vscode.workspace.onDidChangeConfiguration((event) => {
      if (event.affectsConfiguration(BUILD_DIRECTORY_SETTING, folder.uri) ||
          event.affectsConfiguration(DATABASE_SETTING, folder.uri)) {
        activeDatabasePath = null;
        scheduleRefresh();
      }
    })
  );

  context.subscriptions.push(vscode.tasks.registerTaskProvider("cmake-intellisense-sync", {
    provideTasks: () => [createSyncTask()],
    resolveTask: (task) => task.definition.type === "cmake-intellisense-sync"
      ? createSyncTask(task.definition)
      : undefined
  }));

  const watcher = vscode.workspace.createFileSystemWatcher("**/compile_commands.json");
  context.subscriptions.push(watcher);
  context.subscriptions.push(
    watcher.onDidCreate(scheduleRefresh),
    watcher.onDidChange(scheduleRefresh),
    watcher.onDidDelete(scheduleRefresh)
  );
  void refresh();

  function createSyncTask(definition = { type: "cmake-intellisense-sync" }) {
    const execution = new vscode.CustomExecution(async () => {
      const writeEmitter = new vscode.EventEmitter();
      const closeEmitter = new vscode.EventEmitter();
      let closed = false;
      const close = (code) => {
        if (!closed) {
          closed = true;
          closeEmitter.fire(code);
          writeEmitter.dispose();
          closeEmitter.dispose();
        }
      };
      const terminal = {
        onDidWrite: writeEmitter.event,
        onDidClose: closeEmitter.event,
        open: () => {
          writeEmitter.fire("Reading CMake compile_commands.json...\r\n");
          void syncLatestDatabase().then((databasePath) => {
            writeEmitter.fire(`Updated IntelliSense settings from ${databasePath}\r\n`);
            close(0);
          }).catch((error) => {
            const message = error instanceof Error ? error.message : String(error);
            output.appendLine(`Task failed: ${message}`);
            writeEmitter.fire(`Failed to update IntelliSense settings: ${message}\r\n`);
            close(1);
          });
        },
        close: () => close(1)
      };
      return terminal;
    });
    const task = new vscode.Task(
      definition,
      folder,
      "Update C/C++ IntelliSense settings",
      "CMake IntelliSense Sync",
      execution
    );
    task.presentationOptions = {
      clear: true,
      panel: vscode.TaskPanelKind.Dedicated,
      reveal: vscode.TaskRevealKind.Always
    };
    return task;
  }
}

function resolveConfiguredPathForSetting(absolutePath, workspaceFolder) {
  const relativePath = path.relative(workspaceFolder, absolutePath);
  if (relativePath === "" || (!relativePath.startsWith(`..${path.sep}`) && relativePath !== ".." && !path.isAbsolute(relativePath))) {
    return relativePath === "" ? "${workspaceFolder}/compile_commands.json" :
      `\${workspaceFolder}/${relativePath.split(path.sep).join("/")}`;
  }
  return absolutePath;
}

function deactivate() {}

module.exports = { activate, deactivate };
